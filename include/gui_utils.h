#ifndef GUI_UTILS_H
#define GUI_UTILS_H

#include "cm_types.h"

/*
 * Basic drawing primitives of the original, ported from the functions at
 * 0x5e3680..0x5e4890 (gui_utils.cpp, inferred from link order).
 *
 * All coordinates are inclusive. A line, fill, darken, glyph table, image
 * or capture that is partly off screen does nothing. An outline rectangle
 * applies that rule to each side on its own.
 */

typedef struct {
    s32 x1, y1, x2, y2;
} gui_rect_t;

/* gui_line tests GUI_LINE_SOLID only. Without it the line is dashed. */
#define GUI_LINE_DASHED 1
#define GUI_LINE_SOLID  2
#define GUI_RECT_FILL   4

/* Colours that FUN_005e4560 sets up for an RGB565 display. */
#define GUI_COLOUR_NAVY       0x0010 /* DAT_00aebe04, stands in for colour 0 */
#define GUI_COLOUR_YELLOW     0xffe0 /* DAT_00aebdec */
#define GUI_COLOUR_BLACK      0x0000 /* DAT_00aebdea */
#define GUI_COLOUR_LIGHT_GREY 0xe71c /* DAT_00ae3184 */
#define GUI_COLOUR_ORANGE     0xfc00 /* DAT_00aebdd4 */
#define GUI_COLOUR_RED        0xf800 /* DAT_00ae31a8 */
#define GUI_COLOUR_GREY       0x8410 /* DAT_00ae317e */

/* Glyph tables of the original, in gui_glyphs.c. */
extern const u8 gui_glyph_left[];       /* DAT_009cd67c */
extern const u8 gui_glyph_right[];      /* DAT_009cd69c */
extern const u8 gui_glyph_up[];         /* DAT_009cd6bc */
extern const u8 gui_glyph_down[];       /* DAT_009cd6dc */
extern const u8 gui_glyph_tick[];       /* DAT_009cd6fc */
extern const u8 gui_glyph_mark_right[]; /* DAT_009cd730 */
extern const u8 gui_glyph_mark_up[];    /* DAT_009cd774 */
extern const u8 gui_glyph_mark_down[];  /* DAT_009cd7bc */

/* Panel flags. An area and a widget pass their style word straight through. */
#define GUI_PANEL_DARKEN        0x00000002 /* background to 60 percent */
#define GUI_PANEL_GRADIENT_H    0x00000004 /* fades to black, left to right */
#define GUI_PANEL_GRADIENT_V    0x00000008 /* fades to black, top to bottom */
#define GUI_PANEL_FILL          0x00000010
#define GUI_PANEL_BORDER        0x00000020 /* bevel, or outline if flat */
#define GUI_PANEL_SUNKEN        0x00000040 /* swaps the bevel shades */
#define GUI_PANEL_UNDERLINE     0x00000080
#define GUI_PANEL_FLAT          0x00000200
#define GUI_PANEL_LINE          0x00000400 /* corner to corner */
#define GUI_PANEL_OUTER_OUTLINE 0x00000800 /* yellow, one pixel outside */
#define GUI_PANEL_SAMPLE_COLOUR 0x00001000 /* colour of the top-left pixel */
#define GUI_PANEL_DASHED        0x00002000 /* for the outline and the line */
#define GUI_PANEL_LINE_H        0x00004000 /* the line runs across the middle */
#define GUI_PANEL_LINE_TOP      0x00008000 /* the line runs across the top */
#define GUI_PANEL_ELLIPSE       0x00010000 /* FUN_005e7840, not ported */
#define GUI_PANEL_SHRINK        0x00020000 /* right and bottom move in by 2 */
#define GUI_PANEL_INSET         0x00040000 /* left and top move in by 2 */
#define GUI_PANEL_GROOVE        0x01000000 /* two-tone line across the middle */

/* FUN_005e3680. Orders and clamps the box. Returns false if it clamped. */
bool gui_clip(s32 x1, s32 y1, s32 x2, s32 y2, gui_rect_t *out);

/* FUN_005e4890. Scales each channel to percent/100, capped at full. */
u16 gui_scale_colour(u16 colour, u8 percent);

/* FUN_005e3730. A dashed line has 4 pixels on, then 2 off. */
void gui_line(s32 x1, s32 y1, s32 x2, s32 y2, u32 style, u16 colour);

/* FUN_005e3b50. Outline for the two line styles, fill for any other style. */
void gui_rect(s32 x1, s32 y1, s32 x2, s32 y2, u32 style, u16 colour);

/* FUN_005e42e0. Scales every pixel in the box to 60 percent. */
void gui_darken(s32 x1, s32 y1, s32 x2, s32 y2);

/*
 * FUN_005e30e0. Fades the box from black to the picture now in it, in
 * `steps` + 1 frames with a flip after each. A negative `steps` fades from
 * the picture to black. Does nothing when the box is not fully on screen.
 */
void gui_fade(s32 x1, s32 y1, s32 x2, s32 y2, s32 steps);

/* FUN_005e5df0. Draws the background first, then the border or line. */
void gui_panel(s32 x1, s32 y1, s32 x2, s32 y2, u32 flags, u16 colour);

/*
 * FUN_005e3ce0. A glyph table is a width byte, a height byte, then one byte
 * per pixel in row order. Non-zero bytes take the colour.
 */
void gui_draw_glyph(s32 x, s32 y, u16 colour, const u8 *glyph);

/* FUN_005e3fd0. Copies the whole image. Zero pixels are not transparent. */
void gui_blit(s32 x, s32 y, const cm_image_t *img);

/*
 * FUN_005e3dd0. Copies a screen box to an image. Fills `reuse` when it has
 * the same size, otherwise allocates a new image and leaves `reuse` alone.
 * Returns NULL when the box is not fully on screen.
 */
cm_image_t *gui_capture(s32 x1, s32 y1, s32 x2, s32 y2, cm_image_t *reuse);

#endif
