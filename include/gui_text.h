#ifndef GUI_TEXT_H
#define GUI_TEXT_H

#include "cm_types.h"

/*
 * FUN_005e6d80. Draws text in a box. Not ported: the font system comes
 * first. `flags`, `second_colour` and `param` pass through from the widget
 * and their meaning is not known yet.
 */
void gui_text(s32 x1, s32 y1, s32 x2, s32 y2, u32 flags, u16 colour,
              u16 second_colour, const char *text, s32 param);

#endif
