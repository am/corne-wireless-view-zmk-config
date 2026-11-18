/*
 * Simple vertical nice!view status screen: 1-bit icons.
 * The data is generated; see scripts/gen_simple_view_assets.py.
 */

#pragma once

#include <stdint.h>

/* Bitmap icon: rows are padded to a whole byte, the most significant bit is
 * the leftmost pixel. */
struct sv_icon {
    uint8_t width;
    uint8_t height;
    const uint8_t *data;
};

#include "assets_list.h"
