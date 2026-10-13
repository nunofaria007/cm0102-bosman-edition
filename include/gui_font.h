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

#endif
