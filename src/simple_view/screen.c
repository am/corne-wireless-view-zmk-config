/*
 * Simple vertical nice!view status screen: ZMK events and the screen itself.
 * Event handling follows ZMK's stock nice!view widget.
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
#include <zmk/wpm.h>
#include <zmk/events/ble_active_profile_changed.h>
#include <zmk/events/endpoint_changed.h>
#include <zmk/events/layer_state_changed.h>
#include <zmk/events/wpm_state_changed.h>
#else
#include <zmk/split/bluetooth/peripheral.h>
#include <zmk/events/split_peripheral_status_changed.h>
#endif

#include "util.h"
#include "draw.h"

static lv_obj_t *canvas_top;
static lv_obj_t *canvas_middle;
static lv_obj_t *canvas_bottom;
static lv_color_t cbuf_top[CANVAS_SIZE * CANVAS_SIZE];
static lv_color_t cbuf_middle[CANVAS_SIZE * CANVAS_SIZE];
static lv_color_t cbuf_bottom[CANVAS_SIZE * CANVAS_SIZE];

static struct sv_state state = {.peripheral = !SV_CENTRAL};

static void redraw_top(void) {
    if (canvas_top != NULL) {
        sv_draw_top(canvas_top, cbuf_top, &state);
    }
}

static void redraw_middle(void) {
    if (canvas_middle != NULL) {
        sv_draw_middle(canvas_middle, cbuf_middle, &state);
    }
}

static void redraw_bottom(void) {
    if (canvas_bottom != NULL) {
        sv_draw_bottom(canvas_bottom, cbuf_bottom, &state);
    }
}

/* Battery (both halves) */

struct battery_state {
    uint8_t level;
    bool usb_present;
};

static void battery_update_cb(struct battery_state battery) {
    state.battery = battery.level;
    state.charging = battery.usb_present;
    redraw_top();
    if (state.peripheral) {
        redraw_middle();
    }
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
    redraw_top();
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
    const char *label;
};

static void layer_update_cb(struct layer_state layer) {
    state.layer_index = layer.index;
    state.layer_label = layer.label;
    redraw_middle();
}

static struct layer_state layer_get_state(const zmk_event_t *_eh) {
    zmk_keymap_layer_index_t index = zmk_keymap_highest_layer_active();
    return (struct layer_state){
        .index = index,
        .label = zmk_keymap_layer_name(zmk_keymap_layer_index_to_id(index)),
    };
}

ZMK_DISPLAY_WIDGET_LISTENER(sv_layer, struct layer_state, layer_update_cb, layer_get_state)
ZMK_SUBSCRIPTION(sv_layer, zmk_layer_state_changed);

/* WPM */

struct wpm_state {
    uint8_t wpm;
};

static void wpm_update_cb(struct wpm_state wpm) {
    if (wpm.wpm == state.wpm) {
        return;
    }
    state.wpm = wpm.wpm;
    redraw_bottom();
}

static struct wpm_state wpm_get_state(const zmk_event_t *_eh) {
    return (struct wpm_state){.wpm = zmk_wpm_get_state()};
}

ZMK_DISPLAY_WIDGET_LISTENER(sv_wpm, struct wpm_state, wpm_update_cb, wpm_get_state)
ZMK_SUBSCRIPTION(sv_wpm, zmk_wpm_state_changed);

#else /* peripheral */

/* Link to the central half */

struct link_state {
    bool connected;
};

static void link_update_cb(struct link_state link) {
    state.connected = link.connected;
    redraw_top();
}

static struct link_state link_get_state(const zmk_event_t *_eh) {
    return (struct link_state){.connected = zmk_split_bt_peripheral_is_connected()};
}

ZMK_DISPLAY_WIDGET_LISTENER(sv_link, struct link_state, link_update_cb, link_get_state)
ZMK_SUBSCRIPTION(sv_link, zmk_split_peripheral_status_changed);

#endif /* SV_CENTRAL */

static lv_obj_t *create_canvas(lv_obj_t *parent, lv_color_t cbuf[], lv_align_t align,
                               lv_coord_t x) {
    lv_obj_t *canvas = lv_canvas_create(parent);
    lv_obj_align(canvas, align, x, 0);
    lv_canvas_set_buffer(canvas, cbuf, CANVAS_SIZE, CANVAS_SIZE, LV_IMG_CF_TRUE_COLOR);
    return canvas;
}

lv_obj_t *zmk_display_status_screen() {
    lv_obj_t *screen = lv_obj_create(NULL);

    /* Same placement as the stock widget: top tile at the panel's right end.
     * The canvases sit on the screen itself, so no theme padding shifts them. */
    canvas_top = create_canvas(screen, cbuf_top, LV_ALIGN_TOP_RIGHT, 0);
    canvas_middle = create_canvas(screen, cbuf_middle, LV_ALIGN_TOP_LEFT, 24);
    canvas_bottom = create_canvas(screen, cbuf_bottom, LV_ALIGN_TOP_LEFT, -44);

    redraw_top();
    redraw_middle();
    redraw_bottom();

    sv_battery_init();
#if SV_CENTRAL
    sv_output_init();
    sv_layer_init();
    sv_wpm_init();
#else
    sv_link_init();
#endif

    return screen;
}
