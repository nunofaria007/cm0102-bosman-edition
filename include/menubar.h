#ifndef MENUBAR_H
#define MENUBAR_H

#include "cm_types.h"

/*
 * The side bar of Menubar.cpp on the left of each screen. The port has the
 * part for a screen with no manager record: the start screen and its
 * dialogs. The modes 2 to 4, which need a manager record, are not ported.
 */

/* DAT_00b74310. The time of the last side bar build. */
extern s32 g_menubar_time;

/*
 * FUN_0075db00 with no manager record, so mode 1: the side bar picture, the
 * version, "<<<" (id -2) and ">>>" (id -3), "Add Manager" as a label,
 * "Restart Game" (id 0x42f) and "Exit Game" (id 0x402). Restart and Exit are
 * labels while a flagged screen is open, and Restart also for a player other
 * than 0. The "Add Manager" button (id 0x3fb) needs a game and the "Chat"
 * button (id 0x421) a network game: neither is ported.
 */
void menubar_build(u8 mode, s32 arg2);

/*
 * FUN_007615d0 with no manager record. "<<<" goes back and ">>>" forward
 * when the screen stack can. "Restart Game" and "Exit Game" open the
 * "Please Confirm" screen. After that screen closes (after_pop), "Yes" to
 * Exit returns SCREEN_QUIT where the original ends the program, and "Yes" to
 * Restart starts the screen stack again on the start screen where the
 * original starts a new copy of itself. Returns 0 for any other click.
 */
s32 menubar_input(s32 clicked);

/*
 * FUN_007643d0. It handles ids 0x426, 0x7d0 to 0x7e7 and 0x421, which no
 * ported screen has, so the port returns 0.
 */
s32 menubar_input_more(s32 clicked);

#endif
