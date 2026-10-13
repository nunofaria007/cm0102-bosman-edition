#ifndef GUI_FONT_H
#define GUI_FONT_H

#include "cm_types.h"

/*
 * The bitmap fonts of the original, from the functions at 0x5e4a60 and
 * 0x5e4ba0. The original draws text with them when its font setting
 * (DAT_009cc2c4) is -1. Each comment gives the original offset.
 */

#define GUI_FONT_COUNT      8
#define GUI_FONT_FIRST_CHAR 0x20
#define GUI_FONT_CHARS      256

typedef struct {
    s32 width;      /* 0x00 */
    s32 row_bytes;  /* 0x04, bytes in one pixel row, 0 for a glyph with no rows */
    s32 left;       /* 0x08, taken off the gap before the glyph */
    s32 right;      /* 0x0c, taken off the gap after the glyph */
    u8 *rows;       /* 0x10, two pixels per byte, high half first */
} gui_glyph_t;

typedef struct {
    s32 height;                          /* 0x00 */
    gui_glyph_t glyphs[GUI_FONT_CHARS];  /* 0x04 */
} gui_font_t;

/*
 * DAT_00ae1dac. Only fonts 1 to 7 exist. In the original the place of font 0
 * holds the pixel format and the palette, so text in font 0 is undefined
 * there. The port keeps font 0 empty.
 */
extern gui_font_t g_fonts[GUI_FONT_COUNT];

/*
 * FUN_005e4ba0. Loads one .fnt file. The original trusts the file. The port
 * also rejects a height outside 1 to 255 and a glyph whose row does not hold
 * its width.
 */
bool gui_font_load(s32 font, const char *path);

/* The bitmap font part of FUN_005e4a60: loads the seven files of the original. */
bool gui_fonts_load(const char *data_dir);

/*
 * Text rules of the original, for both functions below. "#xxxx#" with four
 * hex digits stands for the character with that code. "|" counts as a space.
 * A character below 0x20 has no width. A code above 0xff reads past the font
 * in the original. The port gives it no glyph. Font -1 means no text.
 */

/* FUN_005e5ad0. Width of the text in pixels. */
s32 gui_line_width(s16 font, const char *text);

/*
 * FUN_005e5160. Draws the text with its top-left corner at (x, y). A glyph
 * that is partly off screen is left out. `cursor` is the index of the
 * character that gets a cursor mark before it, or -1.
 */
void gui_draw_line(s32 x, s32 y, s16 font, u16 colour, const char *text,
                   s32 cursor);

#endif
