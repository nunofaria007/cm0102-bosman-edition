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
 * FUN_0081bd70 for a plain start. "Start New Game" (id 1) runs
 * setup_start_new_game and returns SCREEN_REBUILD, or SCREEN_QUIT when the
 * database does not load, where the original ends the program. The other
 * eight menu items return SCREEN_REBUILD and do nothing else: their screens
 * are not ported. ">>>" (id -3) returns 0. Other clicks go to the side bar.
 */
s32 setup_input(s32 clicked);

/* The two options of "Select League(s)". Both are 1 in the file. */
extern u8 g_real_players;        /* DAT_009b5981, "Use Real Players" */
extern u8 g_attribute_masking;   /* DAT_009cc270, "Attribute Masking" */

/*
 * FUN_0081ce00, "Start New Game". With no nation table in memory, it loads
 * the database from g_data_dir with the "Loading database" progress bar
 * (database_load), selects the nation of the language
 * (setup_default_leagues) and builds the league table. Then it pushes
 * "Select League(s)" and sets slots 0 (scroll position) and 1 (list area)
 * to 0. A second call, after "Back", only pushes the screen again.
 *
 * Returns false when the load or the league table fails. The original then
 * shows an error box and ends the program. Not ported: the free memory check
 * and its "No Memory" screen (the port acts as if memory is enough), the
 * settings apply FUN_005d8480 (it changes nothing for language 0, and its
 * write of game.cfg is left out), and the write of 1 to DAT_00a15704, which
 * already holds 1.
 */
bool setup_start_new_game(void);

/*
 * Build 0x81d010 of "Select League(s)" for a new game (DAT_00dd7118 = 0):
 * the options "Use Real Players" and "Attribute Masking", "Select All" and
 * "De-Select All", and the list of the nations of the league table in
 * screen order, 16 rows in view. Each row has the name (id 0xc),
 * "SELECTED" (0xd), "BACKGROUND" (0xe) and the extra division (0x38) when
 * the nation has one. The data of these buttons is the nation. It also
 * counts g_selected_nations again, and shows the list from the row in
 * slot 0.
 */
void select_leagues_build(void);

/*
 * Input 0x81dc30 for a new game. Saves the first row in view in slot 0,
 * then acts on the id and returns SCREEN_REBUILD_11:
 *   0xa  "Select All" selects every nation with a league that is not
 *        selected, and turns a background league into a selected one.
 *   0xb  "De-Select All" clears every nation.
 *   0xc, 0xd, 0xe, 0x38  select or clear the nation of the button, as a
 *        selected league, a background league or an extra division.
 *   0x39, 0x3a, 0x3b, 0x3c  set the two options.
 * "Next" (-3) returns 0 with no nation selected. Else it returns
 * SCREEN_REBUILD: "Select Start Season" and the game creation of the
 * original are not ported. Other clicks go to the side bar. Returns
 * SCREEN_BACK with no nation table.
 *
 * Not ported: the memory check of the original before a nation is added
 * (the port acts as if memory is enough), and the write of 1 to
 * DAT_00dd7124 before the game creation.
 */
s32 select_leagues_input(s32 clicked);

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
