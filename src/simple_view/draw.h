/*
 * Simple vertical nice!view status screen: drawing.
 *
 * The screen is drawn as a 68x160 portrait image, one bit per pixel, straight
 * from bitmap fonts and icons: nothing is scaled, rotated or smoothed on the
 * way. sv_render() calls sv_px() for every lit pixel; the caller clears its
 * buffer first and maps portrait pixels to the panel.
 */

#pragma once

#include <stdbool.h>
#include <stdint.h>

#define SV_WIDTH 68
#define SV_HEIGHT 160

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
    int profile_count;
    bool connected;
    bool bonded;

    uint8_t layer_index;
    const char *layer_label;
};

/* Provided by the caller: light one pixel of the portrait image. */
void sv_px(int x, int y);

void sv_render(const struct sv_state *state);
