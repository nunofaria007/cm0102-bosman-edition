#ifndef SCRMAN_INPUT_H
#define SCRMAN_INPUT_H

#include "scrman.h"

/*
 * The mouse state of the original. Its window procedure sets these from the
 * Windows messages. The port sets them from SDL events.
 */
extern s32 g_mouse_x;          /* DAT_00b67a20 */
extern s32 g_mouse_y;          /* DAT_00b67a14 */
extern s32 g_left_held;        /* DAT_00b67a34 */
extern s32 g_right_held;       /* DAT_00b67a38 */
extern s32 g_left_clicked;     /* DAT_00b67a0c, set when the button goes down */
extern s32 g_right_clicked;    /* DAT_00b67a1c */
extern s32 g_key_code;         /* DAT_00b67a04, the Windows key code of the last key */
extern u8 g_key_char;          /* DAT_00b67a24, the last character typed */
extern s32 g_cursor_kind;      /* DAT_009c36e8, 2 = over a button */

/*
 * DAT_00b67a3c. The frames that a click of the keys or the wheel lasts. While
 * it is not 0, the window procedure ignores the mouse.
 */
extern s32 g_click_frames;

/*
 * FUN_00560490, the hit test of mode 0. Returns the widget under the mouse
 * while the button is held, or -1. Highlights it and takes the highlight off
 * the others. Rows of the areas come after the widgets with no area, and a
 * later widget wins. After the rows of each area, a held button on its
 * scrollbar (FUN_00561f10) moves the rows: the arrows by one row and the
 * track by one page, once for each call. The thumb starts mode 2. The
 * scrollbar of an area out of view takes clicks too, as in the original.
 *
 * The test runs in two passes. The buttons and the scrollbars count in the
 * first pass only. Each text entry (WIDGET_TEXT_ENTRY) gets
 * scrman_text_entry in both passes, in the same order. When one finishes,
 * the hit test returns it at once. The text entries after it get one more
 * call, so that the one with the passed focus starts. For a free widget
 * these are the rest of the free widgets, then all free widgets again in
 * the first pass. For a row they are the rows of this area and of the
 * areas after it.
 *
 * The opening of a popup area is not ported: a widget with a popup area
 * acts as a plain button.
 */
s16 scrman_hit_test(scrman_t *sm);

/*
 * FUN_005617c0. One step of the text entry widget `index`. A click inside
 * starts the editing, with the cursor at the end of the text. A click
 * outside, Tab or Enter ends it and returns whether the text changed. Tab
 * and Enter also pass the focus to the next text entry that the hit test
 * calls, unless the widget has WIDGET_KEEPS_FOCUS. While the widget edits,
 * Left, Right, Backspace and Delete (g_key_code) move the cursor and delete,
 * and g_key_char goes in at the cursor. A widget with a negative text_limit
 * empties its text at the first key. The text takes no more than text_limit
 * characters and must fit the widget with 10 pixels to spare. It takes no
 * character below a space and none of " * / < > | { } [ ] ~. It takes : and
 * \ only with WIDGET_TEXT_PATH, and ? only with WIDGET_TEXT_QUESTION. A key
 * that the widget uses clears g_key_code and g_key_char. A key that it
 * cannot use stays. Returns false while the widget edits.
 *
 * The original shows an error box for a widget with no WIDGET_TEXT_ENTRY.
 * The port lets the button go and returns false, as the original does
 * after the box. With a text_limit above 255 the original can write past
 * its buffer. The port keeps 255 characters.
 */
bool scrman_text_entry(scrman_t *sm, s16 index);

/*
 * FUN_00561050, the thumb drag of mode 2. Moves the thumb by the change in
 * mouse y, within the track, and the rows with it. The release goes back to
 * mode 0. Returns -1.
 */
s16 scrman_scrollbar_drag(scrman_t *sm);

/*
 * FUN_0055f620. One input step: the hotkey and click results that the
 * screen set, else the input of the current mode. Modes 0 (hit test) and 2
 * (thumb drag) are ported. The popup areas (mode 1), the widget drag
 * (mode 3) and the text timers (FUN_00569610) are not ported.
 */
s16 scrman_poll(scrman_t *sm);

/*
 * FUN_00803390, which only calls FUN_005621f0. The keys and the wheel click
 * a part of g_key_scrollbar: they put the mouse there and hold the button for
 * `frames` frames. Mode 0 clicks the down arrow, 1 the up arrow, 2 the track
 * under the thumb, 3 the top of the track. Any other mode lets the button go.
 * Does nothing when g_key_scrollbar is NULL.
 */
void scrman_scroll_key(s32 mode, s32 frames);

/*
 * The start of the message pump FUN_0068d520: counts g_click_frames down and
 * lets the button go when it reaches 0.
 */
void scrman_count_click_frame(void);

#endif
