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
extern u8 g_key_char;          /* DAT_00b67a24, the last character typed */
extern s32 g_cursor_kind;      /* DAT_009c36e8, 2 = over a button */

/*
 * FUN_00560490, the hit test of mode 0. Returns the widget under the mouse
 * while the button is held, or -1. Highlights it and takes the highlight off
 * the others. Rows of the areas come after the widgets with no area, and a
 * later widget wins. Text entry, the scrollbar click and the opening of a
 * popup area are not ported: a widget with a popup area acts as a plain
 * button.
 */
s16 scrman_hit_test(scrman_t *sm);

/*
 * FUN_0055f620. One input step: the hotkey and click results that the
 * screen set, else the hit test of the current mode. Only mode 0 is ported.
 * The text timers (FUN_00569610) are not ported.
 */
s16 scrman_poll(scrman_t *sm);

#endif
