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

struct nation;

/* The league choice of a new game. */
extern s32 g_leagues_to_load;              /* DAT_00ae2d00 */
extern s32 g_selected_nations;             /* DAT_00ae2d04 */
extern struct nation *g_first_selected;    /* DAT_00ae2d08 */

/*
 * FUN_00827d50. Clears the league flags of every nation, then selects the
 * nation of the language index g_language_index (langlib.h): 0 and 1
 * England, 2 France, 3 Germany, 4 Italy, 5 Spain, 6 Denmark, 7 Holland,
 * 8 Norway, 9 Portugal, 10 Sweden, 11 Japan, 12 South Korea. Returns false
 * for another index or no nation table, where the original shows an error
 * box. Needs key_index_match().
 */
bool setup_default_leagues(void);

#endif
