/*
 * Simple vertical nice!view status screen: drawing.
 *
 * The panel is 160x68 in landscape. Like the stock nice!view widget, the
 * screen is built from three 68x68 canvases that are drawn upright and then
 * rotated, so on an upright panel they read top to bottom:
 *
 *   top     (68x68)  output + battery
 *   middle  (68x68)  layer
 *   bottom  (24 px)  WPM
 */

#pragma once

#include <stdbool.h>
#include <stdint.h>
#include <lvgl.h>

enum sv_output {
    SV_OUTPUT_NONE,
    SV_OUTPUT_USB,
    SV_OUTPUT_BLE,
};

struct sv_state {
    bool peripheral;

    uint8_t battery;
    bool charging;

    /* Central: selected output. Peripheral: link to the central half. */
    enum sv_output output;
    int profile_index;
    bool connected;
    bool bonded;

    uint8_t layer_index;
    const char *layer_label;
    uint8_t wpm;
};

void sv_draw_top(lv_obj_t *canvas, lv_color_t cbuf[], const struct sv_state *state);
void sv_draw_middle(lv_obj_t *canvas, lv_color_t cbuf[], const struct sv_state *state);
void sv_draw_bottom(lv_obj_t *canvas, lv_color_t cbuf[], const struct sv_state *state);
