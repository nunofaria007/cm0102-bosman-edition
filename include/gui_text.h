#ifndef GUI_TEXT_H
#define GUI_TEXT_H

#include "cm_types.h"

/* Text flags. Without a flag the text is centred both ways. */
#define GUI_TEXT_LEFT     0x00000001
#define GUI_TEXT_TOP      0x00000002
#define GUI_TEXT_EMBOSS   0x00000020 /* two shades of the pixel in the box centre */
#define GUI_TEXT_RIGHT    0x00000040
#define GUI_TEXT_PASSWORD 0x00000080 /* a star for each character */
#define GUI_TEXT_WRAP     0x00000100 /* a line that is too wide goes on below */

/*
 * FUN_005e6d80. Draws text in a box, in one of the bitmap fonts. A line break
 * starts a new line. Text that is too wide is cut at its last space, and with
 * GUI_TEXT_WRAP the rest goes to the next line. `cursor` is the index of the
 * character that gets a cursor mark before it in each line, or -1.
 *
 * The original looks for the last space at the place of the character in the
 * whole text, not in the line. From the third line on, this can leave a line
 * too wide and repeat words of the first line. The port does the same. Where
 * the original then reads memory that it never set, the port reads zeros.
 * The port uses the first 251 characters of the text. The original writes
 * past its line buffer for a longer text.
 */
void gui_text(s32 x1, s32 y1, s32 x2, s32 y2, u32 flags, s16 font, u16 colour,
              const char *text, s32 cursor);

#endif
