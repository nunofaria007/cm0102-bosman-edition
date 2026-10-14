#ifndef PROGRESS_BOX_H
#define PROGRESS_BOX_H

#include "screen_stack.h"

/*
 * The progress and message box of the original. It is drawn straight onto
 * the screen over the bottom bar, (100, 555)-(790, 590), and flipped at
 * once. It is not a screen and makes no area or widget, so the next screen
 * draw paints over it.
 *
 * A long job calls screen_stack_progress for each step. The database load
 * of "Start New Game" (FUN_00528540) makes 47 calls, all with the text
 * "Loading database" from text_format and a total of 46:
 *
 *   step 0       at the start (0x5285d4), which also darkens the screen
 *   step 1       after index.dat (0x5289a9)
 *   steps 2..23  one for each of the 22 tables in the sizing loop (0x5289dd
 *                or 0x529930)
 *   steps 24..45 one for each of the 22 tables in the read loop (0x52b3ab)
 *   step 46      after the read loop (0x52c671), a full bar
 *
 * The first two calls pass channel -1. The two loops and the last call pass
 * a register of the loader whose value the research did not trace. For one
 * local player every channel gives the same box, so the port's load can
 * pass -1 throughout:
 *
 *   char text[0x100];
 *   text_format(text, sizeof(text), "Loading database");
 *   for (s32 step = 0; step <= 46; step++) {
 *       screen_stack_progress(&g_screen_stack, text, step, 46, -1);
 *       ... the work of that step ...
 *   }
 *
 * Each call also runs the message pump (g_screen_pump), so the window stays
 * alive during the load. The box does not end the load when the window
 * closes: the main loop sees the close at its next pump.
 */

/*
 * FUN_005e76f0. Draws the box and flips it. With 0 < total and
 * step <= total: `text` in (120, 565)-(250, 580) and a bar in
 * (270, 565)-(770, 580), filled in navy from x = 272 to
 * 272 + step * 496 / total. Otherwise only `text`, across
 * (120, 565)-(770, 580). Font 1, light grey, centred. Step 0 first darkens
 * the whole screen once (scrman_darken_once on the screen manager of the
 * record). Runs the message pump first. Returns 0 and draws nothing when
 * `text` is NULL, else 1.
 *
 * The step times 496 wraps at 32 bits, as in the original. The original
 * also returns 0 at once when its display is off (DAT_00aebe0c). The port
 * has no such switch.
 */
s32 gui_progress_box(const char *text, s32 step, s32 total);

/*
 * FUN_00802f80, the part that a game with one local player runs: opcode 7
 * applied to the screen manager of the record. It sets unknown_12f551 to 0
 * and draws gui_progress_box(text, step, total). Channel -1 sends to every
 * channel with records, so it draws when record_count > 0 and the record is
 * the local player's. Channel 0 draws
 * when the record is the local player's. Channels 1 to 15 have no record in
 * a local game, and the original then applies the opcode to the local
 * screen manager as well. Other channels do nothing.
 *
 * Not ported: the packet buffer that the original fills and reads back, the
 * network send to a remote channel (FUN_0077ac30), and the cursor change
 * (FUN_005e8170(1)) that only runs when DAT_00b77c8c is set, which nothing
 * does. The original fails on a NULL `text`. The port does nothing.
 */
void screen_stack_progress(screen_stack_t *st, const char *text, s32 step,
                           s32 total, s16 channel);

#endif
