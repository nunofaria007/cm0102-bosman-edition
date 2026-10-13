#ifndef GUI_TEXT_H
#define GUI_TEXT_H

#include "cm_types.h"

/*
 * FUN_005e6d80. Draws text in a box, in one of the bitmap fonts. `cursor` is
 * the place of the cursor mark in the text, or -1 for none. Not ported yet.
 */
void gui_text(s32 x1, s32 y1, s32 x2, s32 y2, u32 flags, s16 font, u16 colour,
              const char *text, s32 cursor);

#endif
