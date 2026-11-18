/*
 * Simple vertical nice!view status screen: drawing. See draw.h.
 *
 * Layout (x 2..65 is the content column):
 *
 *   y   4   connection icon (USB / Bluetooth) left, link status right
 *   y  36   one circle per Bluetooth profile, the selected one filled
 *   y  54   ---------
 *           layer name, centred in the band
 *   y 126   ---------
 *   y 132   battery left, bolt right while charging
 */

#include <stdio.h>
#include <string.h>

#include "assets.h"
#include "draw.h"

#define MARGIN 2
#define RIGHT (SV_WIDTH - MARGIN)

#define ROW_OUTPUT 4
#define ROW_PROFILES 36
#define RULE_1 54
#define RULE_2 126
#define ROW_BATTERY 132

#define MAX_PROFILES 5
#define PROFILE_GAP 3

/* Layer names may run one pixel into each margin. */
#define NAME_W (SV_WIDTH - 2)
#define NAME_MAX 24
/* A name this many pixels too wide is tightened instead of shrunk. */
#define MAX_TIGHTEN 3

static void draw_bitmap(int x, int y, int width, int height, const uint8_t *data) {
    int stride = (width + 7) / 8;
    for (int row = 0; row < height; row++) {
        for (int col = 0; col < width; col++) {
            if (data[row * stride + col / 8] & (0x80 >> (col % 8))) {
                sv_px(x + col, y + row);
            }
        }
    }
}

static void draw_icon(int x, int y, const struct sv_icon *icon) {
    draw_bitmap(x, y, icon->width, icon->height, icon->data);
}

static void draw_icon_right(int y, const struct sv_icon *icon) {
    /* Centred on the 24 px row. */
    draw_icon(RIGHT - icon->width, y + (24 - icon->height) / 2, icon);
}

static void draw_rect(int x, int y, int width, int height) {
    for (int row = 0; row < height; row++) {
        for (int col = 0; col < width; col++) {
            sv_px(x + col, y + row);
        }
    }
}

/* Dotted rule across the content column. */
static void draw_rule(int y) {
    for (int x = MARGIN; x < RIGHT; x += 2) {
        sv_px(x, y);
    }
}

static const struct sv_glyph *glyph(const struct sv_font *font, char c) {
    uint8_t code = (uint8_t)c;
    if (code < font->first || code >= font->first + font->count) {
        code = '?';
    }
    return &font->glyphs[code - font->first];
}

/* Width of the lit pixels of a line of text, from the first glyph's left
 * edge to the last glyph's right edge. */
static int text_width(const struct sv_font *font, const char *text) {
    size_t len = strlen(text);
    if (len == 0) {
        return 0;
    }
    int pen = 0;
    for (size_t i = 0; i + 1 < len; i++) {
        pen += glyph(font, text[i])->advance;
    }
    const struct sv_glyph *first = glyph(font, text[0]);
    const struct sv_glyph *last = glyph(font, text[len - 1]);
    return pen + last->x + last->width - first->x;
}

/* Draw text centred on the screen with its baseline at y. The first
 * `tighten` letter pairs are set one pixel closer, to fit a name that is a
 * few pixels too wide. */
static void draw_text_centered(int baseline, const struct sv_font *font, const char *text,
                               int tighten) {
    int width = text_width(font, text) - tighten;
    int pen = (SV_WIDTH - width) / 2 - glyph(font, text[0])->x;

    for (int i = 0; text[i] != '\0'; i++) {
        const struct sv_glyph *g = glyph(font, text[i]);
        draw_bitmap(pen + g->x, baseline - font->ascent + g->top, g->width, g->height,
                    font->data + g->offset);
        pen += g->advance - (i < tighten ? 1 : 0);
    }
}

static void draw_output(const struct sv_state *state) {
    if (state->peripheral) {
        draw_icon(MARGIN, ROW_OUTPUT, &sv_icon_bluetooth);
        draw_icon_right(ROW_OUTPUT, state->connected ? &sv_icon_ok : &sv_icon_off);
    } else if (state->output == SV_OUTPUT_USB) {
        draw_icon(MARGIN, ROW_OUTPUT, &sv_icon_usb);
        draw_icon_right(ROW_OUTPUT, &sv_icon_ok);
    } else if (state->output == SV_OUTPUT_BLE) {
        draw_icon(MARGIN, ROW_OUTPUT, &sv_icon_bluetooth);
        if (!state->bonded) {
            draw_icon_right(ROW_OUTPUT, &sv_icon_pairing);
        } else {
            draw_icon_right(ROW_OUTPUT, state->connected ? &sv_icon_ok : &sv_icon_off);
        }
    }
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

static void draw_layer(const struct sv_state *state) {
    char name[NAME_MAX];
    if (state->layer_label == NULL || state->layer_label[0] == '\0') {
        snprintf(name, sizeof(name), "L%d", state->layer_index);
    } else {
        strncpy(name, state->layer_label, sizeof(name) - 1);
        name[sizeof(name) - 1] = '\0';
    }

    /* One size for every name that fits; step down only for longer names,
     * and cut what does not fit even the smallest font. */
    const struct sv_font *fonts[] = {&sv_font_helvB18, &sv_font_helvB14, &sv_font_helvB10};
    const struct sv_font *font = NULL;
    int tighten = 0;

    for (int i = 0; i < 3 && font == NULL; i++) {
        int over = text_width(fonts[i], name) - NAME_W;
        if (over <= 0) {
            font = fonts[i];
        } else if (over <= MAX_TIGHTEN && over < (int)strlen(name)) {
            font = fonts[i];
            tighten = over;
        }
    }
    if (font == NULL) {
        font = fonts[2];
        for (size_t len = strlen(name); len > 1 && text_width(font, name) > NAME_W; len--) {
            name[len - 1] = '\0';
        }
    }

    /* Centre the capital height in the band between the two rules. */
    int cap = glyph(font, 'H')->height;
    int baseline = (RULE_1 + RULE_2) / 2 + cap / 2;
    draw_text_centered(baseline, font, name, tighten);
}

static void draw_battery(const struct sv_state *state) {
    int level = state->battery > 100 ? 100 : state->battery;

    /* The cell inside the battery outline is 10 px wide; fill it in 2 px
     * steps, and keep one step lit until the battery is truly empty. */
    int fill = (level + 10) / 20 * 2;
    if (fill == 0 && level > 0) {
        fill = 2;
    }
    draw_icon(MARGIN, ROW_BATTERY, &sv_icon_battery);
    draw_rect(MARGIN + 6, ROW_BATTERY + 9, fill, 6);

    if (state->charging) {
        draw_icon_right(ROW_BATTERY, &sv_icon_charging);
    }
}

void sv_render(const struct sv_state *state) {
    draw_output(state);
    draw_rule(RULE_1);
    draw_rule(RULE_2);
    draw_battery(state);

    if (!state->peripheral) {
        draw_profiles(state);
        draw_layer(state);
    }
}
