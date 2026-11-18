/*
 * Simple vertical nice!view status screen: drawing. See draw.h.
 *
 * Central half, top to bottom:
 *
 *   connection icon (USB / Bluetooth) and link status, inset from the edges
 *   one circle per Bluetooth profile, the selected one filled
 *   one tile per keymap layer, stacked, the active one solid
 *   battery: five cells, or a bolt while charging
 *
 * Peripheral half: link icon and status, a field of swell lines, battery.
 *
 * Integer arithmetic only: the keyboard builds without an FPU.
 */

#include <stddef.h>

#include "assets.h"
#include "draw.h"

#define OUTPUT_H 21
#define OUTPUT_INSET 8
#define ROW_OUTPUT 4
#define ROW_PROFILES (ROW_OUTPUT + OUTPUT_H + 8)
#define ROW_BATTERY (SV_HEIGHT - 10 - 12)

/* Width of the battery icon without its terminal. */
#define BATTERY_BODY 25

#define MAX_PROFILES 5
#define PROFILE_GAP 3

static void draw_bitmap(int x, int y, int width, int height, const uint8_t *data) {
    int stride = (width + 7) / 8;
    for (int row = 0; row < height; row++) {
        for (int col = 0; col < width; col++) {
            if (data[row * stride + col / 8] & (0x80 >> (col % 8))) {
                sv_px(x + col, y + row, true);
            }
        }
    }
}

static void draw_icon(int x, int y, const struct sv_icon *icon) {
    draw_bitmap(x, y, icon->width, icon->height, icon->data);
}

/* Draw an icon after clearing `border` pixels around each of its pixels. */
static void draw_icon_cutout(int x, int y, const struct sv_icon *icon, int border) {
    int stride = (icon->width + 7) / 8;
    for (int row = 0; row < icon->height; row++) {
        for (int col = 0; col < icon->width; col++) {
            if (!(icon->data[row * stride + col / 8] & (0x80 >> (col % 8)))) {
                continue;
            }
            for (int dy = -border; dy <= border; dy++) {
                for (int dx = -border; dx <= border; dx++) {
                    sv_px(x + col + dx, y + row + dy, false);
                }
            }
        }
    }
    draw_icon(x, y, icon);
}

static void draw_rect(int x, int y, int width, int height) {
    for (int row = 0; row < height; row++) {
        for (int col = 0; col < width; col++) {
            sv_px(x + col, y + row, true);
        }
    }
}

static void draw_output(const struct sv_state *state) {
    const struct sv_icon *icon = NULL;
    const struct sv_icon *status = NULL;

    if (state->peripheral) {
        icon = &sv_icon_bluetooth;
        status = state->connected ? &sv_icon_linked : &sv_icon_unlinked;
    } else if (state->output == SV_OUTPUT_USB) {
        icon = &sv_icon_usb;
        status = &sv_icon_linked;
    } else if (state->output == SV_OUTPUT_BLE) {
        icon = &sv_icon_bluetooth;
        if (!state->bonded) {
            status = &sv_icon_pairing;
        } else {
            status = state->connected ? &sv_icon_linked : &sv_icon_unlinked;
        }
    }
    if (icon == NULL) {
        return;
    }

    /* Both sit inset from the edges, centred on the row. */
    draw_icon(OUTPUT_INSET, ROW_OUTPUT + (OUTPUT_H - icon->height) / 2, icon);
    draw_icon(SV_WIDTH - OUTPUT_INSET - status->width,
              ROW_OUTPUT + (OUTPUT_H - status->height) / 2, status);
}

static void draw_profiles(const struct sv_state *state) {
    int count = state->profile_count > MAX_PROFILES ? MAX_PROFILES : state->profile_count;
    if (count <= 0) {
        return;
    }
    int size = sv_icon_profile.width;
    int x = (SV_WIDTH - (count * size + (count - 1) * PROFILE_GAP)) / 2;

    for (int i = 0; i < count; i++, x += size + PROFILE_GAP) {
        draw_icon(x, ROW_PROFILES,
                  i == state->profile_index ? &sv_icon_profile_active : &sv_icon_profile);
    }
}

/*
 * Layer stack: one flat square per keymap layer seen from above at an angle,
 * the base layer at the bottom. Every edge is a 2:1 pixel line (two pixels
 * across for each pixel down), so the shapes have no uneven steps.
 *
 * Row i of a tile is 4 * (i + 1) pixels wide down to the middle, then narrows
 * again: TILE_N rows each way.
 */
#define TILE_N 14
#define TILE_W (4 * TILE_N)
#define TILE_H (2 * TILE_N)
#define TILE_STEP 12
#define TILE_LINE 2

/* Half the width of tile row `row`, or 0 outside the tile. */
static int tile_half(int row) {
    if (row < 0 || row >= TILE_H) {
        return 0;
    }
    return row < TILE_N ? 2 * (row + 1) : 2 * (TILE_H - row);
}

enum tile_style {
    TILE_ACTIVE,   /* solid */
    TILE_INACTIVE, /* outline */
    TILE_TOP,      /* the highest layer: horizontal lines, no outline */
};

static void draw_tile(int cx, int top, enum tile_style style) {
    for (int row = 0; row < TILE_H; row++) {
        int half = tile_half(row);
        /* A pixel is on the outline when the tile does not reach it
         * TILE_LINE rows further up or down. */
        int inner_up = tile_half(row - TILE_LINE);
        int inner_down = tile_half(row + TILE_LINE);
        int inner = inner_up < inner_down ? inner_up : inner_down;

        for (int dx = -half; dx < half; dx++) {
            int ax = dx < 0 ? -dx - 1 : dx; /* distance from the centre line */
            bool on;

            if (style == TILE_ACTIVE) {
                on = true;
            } else if (style == TILE_INACTIVE) {
                on = ax >= inner;
            } else {
                on = row % 2 == 0;
            }
            /* Clearing as well as setting: a tile hides what is under it. */
            sv_px(cx + dx, top + row, on);
        }
    }
}

/*
 * Tile that stands for a keymap layer, counted from the bottom. In keymap
 * order the base layer is the bottom tile. With base_second the base layer
 * is the second tile and the first layer above it in the keymap (usually
 * "lower") is the one beneath; higher layers follow in keymap order.
 */
static int layer_tile(const struct sv_state *state) {
    if (state->layer_base_second && state->layer_count >= 2 && state->layer_index < 2) {
        return 1 - state->layer_index;
    }
    return state->layer_index;
}

static void draw_layers(const struct sv_state *state, int band_top, int band_bottom) {
    int count = state->layer_count;
    if (count < 1) {
        return;
    }
    int active = layer_tile(state);
    int step = TILE_STEP;
    while (count > 1 && TILE_H + (count - 1) * step > band_bottom - band_top && step > 4) {
        step--;
    }
    int height = TILE_H + (count - 1) * step;
    int bottom_tile = band_top + (band_bottom - band_top - height) / 2 + (count - 1) * step;

    /* Bottom tile first; each tile covers the ones under it. */
    for (int i = 0; i < count; i++) {
        enum tile_style style = TILE_INACTIVE;
        if (i == active) {
            style = TILE_ACTIVE;
        } else if (i == count - 1) {
            style = TILE_TOP;
        }
        draw_tile(SV_WIDTH / 2, bottom_tile - i * step, style);
    }
}

static void draw_battery(const struct sv_state *state) {
    int x = (SV_WIDTH - sv_icon_battery.width) / 2;
    draw_icon(x, ROW_BATTERY, &sv_icon_battery);

    if (state->charging) {
        /* The bolt replaces the cells, with a clear pixel around it where it
         * meets the outline. */
        draw_icon_cutout(x + (BATTERY_BODY - sv_icon_charging.width) / 2,
                         ROW_BATTERY + (sv_icon_battery.height - sv_icon_charging.height) / 2,
                         &sv_icon_charging, 1);
        return;
    }

    /* Five cells of 20 % each; the first stays lit until the battery is
     * truly empty. */
    int level = state->battery > 100 ? 100 : state->battery;
    int cells = (level + 19) / 20;
    for (int i = 0; i < cells; i++) {
        draw_rect(x + 3 + i * 4, ROW_BATTERY + 3, 3, 6);
    }
}

/*
 * Swell: wavy lines stacked down the screen, nearer lines hiding the ones
 * behind them. Each line is the sum of three sine waves of different lengths
 * that move at different speeds as `frame` advances, and the whole field
 * drifts down one pixel every two frames. SV_SWELL_FRAMES frames make a
 * seamless loop.
 *
 * Angles are 16-bit, 65536 to a turn; heights are in 1/64 pixel.
 */
#define SWELL_TOP 42
#define SWELL_BOTTOM 130 /* first row below the field */
#define SWELL_STEP 8     /* rows between lines */
#define SWELL_PHASES 8   /* lines until the shapes repeat */
#define SWELL_FIRST (-2)
#define SWELL_LAST 26

#define TURN 65536
#define FP 64

/* sin() over one turn in 256 steps, scaled to +-127. */
static const int8_t sine[256] = {
       0,    3,    6,    9,   12,   16,   19,   22,   25,   28,   31,   34,
      37,   40,   43,   46,   49,   51,   54,   57,   60,   63,   65,   68,
      71,   73,   76,   78,   81,   83,   85,   88,   90,   92,   94,   96,
      98,  100,  102,  104,  106,  107,  109,  111,  112,  113,  115,  116,
     117,  118,  120,  121,  122,  122,  123,  124,  125,  125,  126,  126,
     126,  127,  127,  127,  127,  127,  127,  127,  126,  126,  126,  125,
     125,  124,  123,  122,  122,  121,  120,  118,  117,  116,  115,  113,
     112,  111,  109,  107,  106,  104,  102,  100,   98,   96,   94,   92,
      90,   88,   85,   83,   81,   78,   76,   73,   71,   68,   65,   63,
      60,   57,   54,   51,   49,   46,   43,   40,   37,   34,   31,   28,
      25,   22,   19,   16,   12,    9,    6,    3,    0,   -3,   -6,   -9,
     -12,  -16,  -19,  -22,  -25,  -28,  -31,  -34,  -37,  -40,  -43,  -46,
     -49,  -51,  -54,  -57,  -60,  -63,  -65,  -68,  -71,  -73,  -76,  -78,
     -81,  -83,  -85,  -88,  -90,  -92,  -94,  -96,  -98, -100, -102, -104,
    -106, -107, -109, -111, -112, -113, -115, -116, -117, -118, -120, -121,
    -122, -122, -123, -124, -125, -125, -126, -126, -126, -127, -127, -127,
    -127, -127, -127, -127, -126, -126, -126, -125, -125, -124, -123, -122,
    -122, -121, -120, -118, -117, -116, -115, -113, -112, -111, -109, -107,
    -106, -104, -102, -100,  -98,  -96,  -94,  -92,  -90,  -88,  -85,  -83,
     -81,  -78,  -76,  -73,  -71,  -68,  -65,  -63,  -60,  -57,  -54,  -51,
     -49,  -46,  -43,  -40,  -37,  -34,  -31,  -28,  -25,  -22,  -19,  -16,
     -12,   -9,   -6,   -3,
};

static int sin127(int32_t angle) { return sine[((uint32_t)angle >> 8) & 0xff]; }

/* Height of line `k` above its baseline at column x, in 1/64 pixel. */
static int swell_height(int x, int k, int frame) {
    int32_t t = frame * (TURN / (SV_SWELL_FRAMES / 2));
    int32_t line = k * (TURN / SWELL_PHASES);

    int swing = (FP * 24 / 10) + (FP * 14 / 10) * sin127(t + line) / 127;
    int height = FP * 35 / 10;
    height += swing * sin127(x * TURN / 40 + line - t) / 127;
    height += FP * sin127(x * TURN / 20 - line + 2 * t) / 127;
    height += (FP * 14 / 10) * sin127(x * TURN / 68 + 2 * line + t) / 127;
    return height;
}

/* Lines thin out to dashes over the last rows before the icons. */
static bool swell_visible(int x, int y) {
    int edge = y - SWELL_TOP < SWELL_BOTTOM - 1 - y ? y - SWELL_TOP : SWELL_BOTTOM - 1 - y;
    if (edge < 0) {
        return false;
    }
    int dash = edge < 5 ? 2 : edge < 10 ? 3 : 4;
    return x % 4 < dash;
}

static void draw_swell(int frame) {
    /* Highest row drawn so far in each column: lines behind stay under it. */
    int16_t horizon[SV_WIDTH];
    for (int x = 0; x < SV_WIDTH; x++) {
        horizon[x] = SWELL_BOTTOM;
    }

    frame %= SV_SWELL_FRAMES;
    if (frame < 0) {
        frame += SV_SWELL_FRAMES;
    }

    /* Nearest line first. */
    for (int k = SWELL_FIRST; k <= SWELL_LAST; k++) {
        int base = SV_HEIGHT + 6 - k * SWELL_STEP + frame / 2;
        int16_t rows[SV_WIDTH];

        for (int x = 0; x < SV_WIDTH; x++) {
            rows[x] = base - ((swell_height(x, k, frame) + FP / 2) >> 6);
        }
        for (int x = 0; x < SV_WIDTH; x++) {
            /* Join to the previous column so the line has no gaps. */
            int from = rows[x], to = rows[x];
            if (x > 0 && rows[x - 1] < rows[x] - 1) {
                from = rows[x - 1] + 1;
            } else if (x > 0 && rows[x - 1] > rows[x] + 1) {
                to = rows[x - 1] - 1;
            }
            for (int row = from; row <= to; row++) {
                if (row < horizon[x] && swell_visible(x, row)) {
                    sv_px(x, row, true);
                }
            }
        }
        for (int x = 0; x < SV_WIDTH; x++) {
            if (rows[x] < horizon[x]) {
                horizon[x] = rows[x];
            }
        }
    }
}

void sv_render(const struct sv_state *state) {
    draw_output(state);
    draw_battery(state);

    if (state->peripheral) {
        draw_swell(state->frame);
    } else {
        draw_profiles(state);
        draw_layers(state, ROW_PROFILES + 10, ROW_BATTERY);
    }
}
