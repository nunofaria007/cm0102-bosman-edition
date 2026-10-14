#ifndef GUI_MENU_H
#define GUI_MENU_H

#include "cm_types.h"

/*
 * Menus, button bars and text boxes of gui_utils.cpp. They create their
 * areas and widgets in the screen manager of the screen stack
 * (g_screen_stack).
 */

/* One item of a menu, 0xd4 bytes in the original. */
typedef struct {
    s16 font;       /* 0x00, -1 = the default font of the menu */
    u16 colour;     /* 0x02, text colour, 0 = the default colour */
    char text[200]; /* 0x04 */
    s32 id;         /* 0xcc, widget +0x08 */
    s32 data;       /* 0xd0, widget +0x48 */
} gui_menu_item_t;

/* FUN_004568f0. Sets the font to -1 and the colour to 0, and nothing else. */
void gui_menu_item_init(gui_menu_item_t *item);

/*
 * FUN_005ed300. A menu of `count` buttons in a new area with `columns`
 * columns and `rows` rows, filled row by row. With `sort`, the items go in
 * the order of their text. A button whose bit is set in `bold_mask` gets an
 * outline and yellow text, the others light grey text. A last row with fewer
 * items than columns is centred. Returns 1, or 0 when there are no items.
 * The original shows an error box then (gui_utils.cpp 193). With 0 columns
 * the original divides by zero. The port returns 0.
 */
s32 gui_menu_build(s32 x1, s32 y1, s32 x2, s32 y2, s32 columns, s32 rows,
                   s32 default_font, s32 count, const gui_menu_item_t *items,
                   u32 bold_mask, s32 sort);

/*
 * FUN_005edcc0. The bar of "Back" (id -2) and "Next" (id -3) under a screen.
 * A cell whose argument is 0 is a label. Returns 1.
 */
s32 gui_back_next_bar(s32 back, s32 next);

/*
 * FUN_005ed070 for a text with no substitutions. Creates an area with one
 * column and `rows` rows, then puts the text in the middle row, centred.
 * Returns 1. NULL text gives the area only.
 *
 * A text wider than the area less 9 pixels, or with a line break, goes
 * through the line breaking of FUN_004aeaf0 in the original. That path and
 * the substitutions after the text argument are not ported: the port then
 * creates the area only.
 */
s32 gui_text_box(s32 x1, s32 y1, s32 x2, s32 y2, u32 flags, u16 colour,
                 s32 rows, s16 font, u16 text_colour, const char *text);

#endif
