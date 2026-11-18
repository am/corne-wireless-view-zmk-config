/*
 * Simple vertical nice!view status screen: drawing. See draw.h.
 */

#include <stdio.h>
#include <string.h>

#include <zephyr/sys/util.h>

#include "util.h"
#include "draw.h"

#define TEXT_W (CANVAS_SIZE - 2)

static void fill_background(lv_obj_t *canvas) {
    lv_draw_rect_dsc_t bg;
    init_rect_dsc(&bg, LVGL_BACKGROUND);
    lv_canvas_draw_rect(canvas, 0, 0, CANVAS_SIZE, CANVAS_SIZE, &bg);
}

/* Same thresholds as ZMK's built-in battery widget. */
static const char *battery_symbol(uint8_t level) {
    if (level > 95) {
        return LV_SYMBOL_BATTERY_FULL;
    }
    if (level > 65) {
        return LV_SYMBOL_BATTERY_3;
    }
    if (level > 35) {
        return LV_SYMBOL_BATTERY_2;
    }
    if (level > 5) {
        return LV_SYMBOL_BATTERY_1;
    }
    return LV_SYMBOL_BATTERY_EMPTY;
}

static lv_coord_t text_width(const char *text, const lv_font_t *font) {
    lv_point_t size;
    lv_txt_get_size(&size, text, font, 0, 0, LV_COORD_MAX, LV_TEXT_FLAG_NONE);
    return size.x;
}

void sv_draw_top(lv_obj_t *canvas, lv_color_t cbuf[], const struct sv_state *state) {
    lv_draw_label_dsc_t big_left;
    init_label_dsc(&big_left, LVGL_FOREGROUND, &lv_font_montserrat_26, LV_TEXT_ALIGN_LEFT);
    lv_draw_label_dsc_t big_right;
    init_label_dsc(&big_right, LVGL_FOREGROUND, &lv_font_montserrat_26, LV_TEXT_ALIGN_RIGHT);
    lv_draw_label_dsc_t small_right;
    init_label_dsc(&small_right, LVGL_FOREGROUND, &lv_font_montserrat_16, LV_TEXT_ALIGN_RIGHT);

    fill_background(canvas);

    /* Row 1: output, with a small status flag on the right for Bluetooth. */
    char output[16] = "";
    const char *flag = NULL;

    if (state->peripheral) {
        strcpy(output, LV_SYMBOL_BLUETOOTH);
        flag = state->connected ? LV_SYMBOL_OK : LV_SYMBOL_CLOSE;
    } else if (state->output == SV_OUTPUT_USB) {
        strcpy(output, LV_SYMBOL_USB);
    } else if (state->output == SV_OUTPUT_BLE) {
        snprintf(output, sizeof(output), LV_SYMBOL_BLUETOOTH " %d", state->profile_index + 1);
        if (!state->bonded) {
            flag = LV_SYMBOL_SETTINGS;
        } else {
            flag = state->connected ? LV_SYMBOL_OK : LV_SYMBOL_CLOSE;
        }
    }

    lv_canvas_draw_text(canvas, 1, 2, TEXT_W, &big_left, output);
    if (flag != NULL) {
        lv_canvas_draw_text(canvas, 1, 8, TEXT_W, &small_right, flag);
    }

    /* Row 2: charging bolt and battery, right aligned. */
    char battery[16];
    snprintf(battery, sizeof(battery), "%s%s", state->charging ? LV_SYMBOL_CHARGE " " : "",
             battery_symbol(state->battery));
    lv_canvas_draw_text(canvas, 1, 36, TEXT_W, &big_right, battery);

    rotate_canvas(canvas, cbuf);
}

void sv_draw_middle(lv_obj_t *canvas, lv_color_t cbuf[], const struct sv_state *state) {
    lv_draw_label_dsc_t label;
    init_label_dsc(&label, LVGL_FOREGROUND, &lv_font_montserrat_26, LV_TEXT_ALIGN_CENTER);

    fill_background(canvas);

    if (state->peripheral) {
        /* Peripheral: battery percentage, large. */
        char percent[8];
        snprintf(percent, sizeof(percent), "%d%%", state->battery);
        lv_canvas_draw_text(canvas, 1, 20, TEXT_W, &label, percent);
        rotate_canvas(canvas, cbuf);
        return;
    }

    /* Keyboard icon, then the layer name in the largest font that fits. */
    lv_canvas_draw_text(canvas, 1, 2, TEXT_W, &label, LV_SYMBOL_KEYBOARD);

    char fallback[12];
    const char *name = state->layer_label;
    if (name == NULL || name[0] == '\0') {
        snprintf(fallback, sizeof(fallback), "Layer %d", state->layer_index);
        name = fallback;
    }

    const lv_font_t *fonts[] = {&lv_font_montserrat_26, &lv_font_montserrat_18,
                                &lv_font_montserrat_16};
    const lv_font_t *font = fonts[ARRAY_SIZE(fonts) - 1];
    for (int i = 0; i < ARRAY_SIZE(fonts); i++) {
        if (text_width(name, fonts[i]) <= TEXT_W) {
            font = fonts[i];
            break;
        }
    }
    label.font = font;

    /* Centre the line in the lower half of the tile (y 34..67). */
    lv_coord_t y = 34 + (34 - lv_font_get_line_height(font)) / 2;
    lv_canvas_draw_text(canvas, 1, y, TEXT_W, &label, name);

    rotate_canvas(canvas, cbuf);
}

void sv_draw_bottom(lv_obj_t *canvas, lv_color_t cbuf[], const struct sv_state *state) {
    lv_draw_label_dsc_t label;
    init_label_dsc(&label, LVGL_FOREGROUND, &lv_font_montserrat_18, LV_TEXT_ALIGN_CENTER);

    fill_background(canvas);

    /* Only the first 24 rows of this tile are on the panel. */
    if (!state->peripheral) {
        char wpm[8];
        snprintf(wpm, sizeof(wpm), "%d", state->wpm);
        lv_canvas_draw_text(canvas, 1, 2, TEXT_W, &label, wpm);
    }

    rotate_canvas(canvas, cbuf);
}
