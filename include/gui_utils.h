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
