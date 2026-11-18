/*
 * Simple vertical nice!view status screen: ZMK events and the screen itself.
 * Event handling follows ZMK's stock nice!view widget.
 *
 * The panel is 160x68 in landscape and sits upright on the keyboard, so the
 * 68x160 portrait image from draw.c is written into one full-screen canvas
 * turned a quarter: the portrait's top edge is the panel's right edge.
 */

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

#include <zmk/battery.h>
#include <zmk/display.h>
#include <zmk/event_manager.h>
#include <zmk/events/battery_state_changed.h>
#include <zmk/events/usb_conn_state_changed.h>
#include <zmk/usb.h>

#define SV_CENTRAL (!IS_ENABLED(CONFIG_ZMK_SPLIT) || IS_ENABLED(CONFIG_ZMK_SPLIT_ROLE_CENTRAL))

#if SV_CENTRAL
#include <zmk/ble.h>
#include <zmk/endpoints.h>
#include <zmk/keymap.h>
#include <zmk/events/ble_active_profile_changed.h>
#include <zmk/events/endpoint_changed.h>
#include <zmk/events/layer_state_changed.h>
#else
#include <zmk/split/bluetooth/peripheral.h>
#include <zmk/events/split_peripheral_status_changed.h>
#endif

#include <lvgl.h>

#include "draw.h"

#define PANEL_W 160
#define PANEL_H 68

#define FOREGROUND                                                                                 \
    (IS_ENABLED(CONFIG_NICE_VIEW_WIDGET_INVERTED) ? lv_color_white() : lv_color_black())
#define BACKGROUND                                                                                 \
    (IS_ENABLED(CONFIG_NICE_VIEW_WIDGET_INVERTED) ? lv_color_black() : lv_color_white())

static lv_obj_t *canvas;
static lv_color_t cbuf[PANEL_W * PANEL_H];

static struct sv_state state = {.peripheral = !SV_CENTRAL};

void sv_px(int x, int y, bool on) {
    if (x < 0 || x >= SV_WIDTH || y < 0 || y >= SV_HEIGHT) {
        return;
    }
    cbuf[x * PANEL_W + (PANEL_W - 1 - y)] = on ? FOREGROUND : BACKGROUND;
}

static void redraw(void) {
    if (canvas == NULL) {
        return;
    }
    lv_color_t background = BACKGROUND;
    for (int i = 0; i < PANEL_W * PANEL_H; i++) {
        cbuf[i] = background;
    }
    sv_render(&state);
    lv_obj_invalidate(canvas);
}

/* Battery (both halves) */

struct battery_state {
    uint8_t level;
    bool usb_present;
};

static void battery_update_cb(struct battery_state battery) {
    state.battery = battery.level;
    state.charging = battery.usb_present;
    redraw();
}

static struct battery_state battery_get_state(const zmk_event_t *eh) {
    const struct zmk_battery_state_changed *ev = as_zmk_battery_state_changed(eh);
    return (struct battery_state){
        .level = (ev != NULL) ? ev->state_of_charge : zmk_battery_state_of_charge(),
#if IS_ENABLED(CONFIG_USB_DEVICE_STACK)
        .usb_present = zmk_usb_is_powered(),
#endif
    };
}

ZMK_DISPLAY_WIDGET_LISTENER(sv_battery, struct battery_state, battery_update_cb,
                            battery_get_state)
ZMK_SUBSCRIPTION(sv_battery, zmk_battery_state_changed);
#if IS_ENABLED(CONFIG_USB_DEVICE_STACK)
ZMK_SUBSCRIPTION(sv_battery, zmk_usb_conn_state_changed);
#endif

#if SV_CENTRAL

/* Output */

struct output_state {
    enum sv_output output;
    int profile_index;
    bool connected;
    bool bonded;
};

static void output_update_cb(struct output_state output) {
    state.output = output.output;
    state.profile_index = output.profile_index;
    state.connected = output.connected;
    state.bonded = output.bonded;
    state.profile_count = IS_ENABLED(CONFIG_ZMK_BLE) ? ZMK_BLE_PROFILE_COUNT : 0;
    redraw();
}

static struct output_state output_get_state(const zmk_event_t *_eh) {
    struct zmk_endpoint_instance endpoint = zmk_endpoints_selected();
    struct output_state output = {.output = SV_OUTPUT_NONE};

    switch (endpoint.transport) {
    case ZMK_TRANSPORT_USB:
        output.output = SV_OUTPUT_USB;
        break;
    case ZMK_TRANSPORT_BLE:
        output.output = SV_OUTPUT_BLE;
        break;
    }

#if IS_ENABLED(CONFIG_ZMK_BLE)
    output.profile_index = zmk_ble_active_profile_index();
    output.connected = zmk_ble_active_profile_is_connected();
    output.bonded = !zmk_ble_active_profile_is_open();
#endif
    return output;
}

ZMK_DISPLAY_WIDGET_LISTENER(sv_output, struct output_state, output_update_cb, output_get_state)
ZMK_SUBSCRIPTION(sv_output, zmk_endpoint_changed);
#if IS_ENABLED(CONFIG_USB_DEVICE_STACK)
ZMK_SUBSCRIPTION(sv_output, zmk_usb_conn_state_changed);
#endif
#if IS_ENABLED(CONFIG_ZMK_BLE)
ZMK_SUBSCRIPTION(sv_output, zmk_ble_active_profile_changed);
#endif

/* Layer */

struct layer_state {
    uint8_t index;
};

static void layer_update_cb(struct layer_state layer) {
    state.layer_index = layer.index;
    state.layer_count = ZMK_KEYMAP_LAYERS_LEN;
    state.layer_base_second = IS_ENABLED(CONFIG_NICE_VIEW_SIMPLE_BASE_LAYER_SECOND);
    redraw();
}

static struct layer_state layer_get_state(const zmk_event_t *_eh) {
    return (struct layer_state){.index = zmk_keymap_highest_layer_active()};
}

ZMK_DISPLAY_WIDGET_LISTENER(sv_layer, struct layer_state, layer_update_cb, layer_get_state)
ZMK_SUBSCRIPTION(sv_layer, zmk_layer_state_changed);

#else /* peripheral */

/* Link to the central half */

struct link_state {
    bool connected;
};

/*
 * Swell animation. It plays one loop when the screen starts (power on, reset,
 * waking from deep sleep) and when the link to the central half comes up,
 * then rests on its first frame so that nothing is redrawn while typing.
 *
 * It runs as an LVGL timer, on the display work queue with everything else
 * that draws, and the timer is paused whenever the animation is not playing.
 */
#if IS_ENABLED(CONFIG_NICE_VIEW_SIMPLE_ANIMATION)

static lv_timer_t *swell_timer;
static int swell_frames_left;

static void swell_tick(lv_timer_t *timer) {
    if (--swell_frames_left <= 0) {
        state.frame = 0;
        lv_timer_pause(timer);
    } else {
        state.frame++;
    }
    redraw();
}

static void swell_play(void) {
    if (swell_timer == NULL) {
        return;
    }
    if (swell_frames_left <= 0) {
        state.frame = 0;
        lv_timer_resume(swell_timer);
    }
    /* Already playing: carry on from the current frame to the end of a loop. */
    swell_frames_left = SV_SWELL_FRAMES - state.frame % SV_SWELL_FRAMES;
}

static void swell_init(void) {
    swell_timer = lv_timer_create(swell_tick, CONFIG_NICE_VIEW_SIMPLE_ANIMATION_FRAME_MS, NULL);
    lv_timer_pause(swell_timer);
    swell_play();
}

#else

static void swell_play(void) {}
static void swell_init(void) {}

#endif /* CONFIG_NICE_VIEW_SIMPLE_ANIMATION */

static void link_update_cb(struct link_state link) {
    bool came_up = link.connected && !state.connected;
    state.connected = link.connected;
    redraw();
    if (came_up) {
        swell_play();
    }
}

static struct link_state link_get_state(const zmk_event_t *_eh) {
    return (struct link_state){.connected = zmk_split_bt_peripheral_is_connected()};
}

ZMK_DISPLAY_WIDGET_LISTENER(sv_link, struct link_state, link_update_cb, link_get_state)
ZMK_SUBSCRIPTION(sv_link, zmk_split_peripheral_status_changed);

#endif /* SV_CENTRAL */

lv_obj_t *zmk_display_status_screen() {
    lv_obj_t *screen = lv_obj_create(NULL);

    canvas = lv_canvas_create(screen);
    lv_obj_align(canvas, LV_ALIGN_TOP_LEFT, 0, 0);
    lv_canvas_set_buffer(canvas, cbuf, PANEL_W, PANEL_H, LV_IMG_CF_TRUE_COLOR);
    redraw();

    sv_battery_init();
#if SV_CENTRAL
    sv_output_init();
    sv_layer_init();
#else
    sv_link_init();
    swell_init();
#endif

    return screen;
}
