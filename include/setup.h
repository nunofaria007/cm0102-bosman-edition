#ifndef SETUP_H
#define SETUP_H

#include "cm_types.h"

/*
 * The start screen "Setup Game" of Setup.cpp: its menu, the side bar and the
 * Back and Next bar.
 */

/*
 * The push of FUN_0081b890: the start screen on g_screen_stack, with no
 * leave or exit function and no flag. The first-run flag that sets slot 0
 * and the command-line loads of the original are not ported.
 */
void setup_push_start_screen(void);

/* FUN_00839570. Formats "Championship Manager 2001/02" into g_title_buffer. */
const char *setup_title(void);

/* FUN_0081bab0. */
void setup_build(void);

/*
 * FUN_0081bd70 for a plain start. The nine menu items return SCREEN_REBUILD
 * and do nothing else: their screens are not ported. ">>>" (id -3) returns 0.
 * Other clicks go to the side bar.
 */
s32 setup_input(s32 clicked);

#endif
