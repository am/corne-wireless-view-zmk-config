/*
 * Simple vertical nice!view status screen: 1-bit fonts and icons.
 * The data is generated; see scripts/gen_simple_view_assets.py.
 */

#pragma once

#include <stdint.h>

/* One glyph of a bitmap font. Rows are padded to a whole byte, the most
 * significant bit is the leftmost pixel. */
struct sv_glyph {
    uint16_t offset; /* into the font's data */
    uint8_t width;   /* of the glyph's bitmap */
    uint8_t height;
    int8_t x;        /* bitmap's left edge, from the pen position */
    int8_t top;      /* bitmap's first row, down from the ascent line */
    uint8_t advance; /* pen movement to the next glyph */
};

struct sv_font {
    uint8_t ascent;
    uint8_t descent;
    uint8_t first;
    uint8_t count;
    const struct sv_glyph *glyphs;
    const uint8_t *data;
};

/* Bitmap icon, rows packed like font glyphs. */
struct sv_icon {
    uint8_t width;
    uint8_t height;
    const uint8_t *data;
};

#include "assets_list.h"
