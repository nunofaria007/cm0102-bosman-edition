#ifndef KEY_NATION_H
#define KEY_NATION_H

#include "database.h"

/*
 * The league table of key_nation.cpp: the 35 competitions that a new game
 * can run, with their order on "Select League(s)" and the text of the extra
 * division.
 *
 * API for "Select League(s)", after database_load_nations() and
 * key_index_match():
 *   league_table_build()        fills g_leagues. Returns false when no nation
 *                               table is loaded.
 *   setup_default_leagues()     (setup.h) selects the nation of the language.
 *   league_order(0)             the 35 rows of g_leagues in screen order.
 *                               Skip a row whose nation is NULL.
 *   league_extra_division(n, buf, size)
 *                               the text of the third button of a row, such
 *                               as "Conference Division", when it returns 1.
 */

#define LEAGUE_COUNT 35

/*
 * One entry, 0x48 bytes in the original (0xb63d60). The port keeps the
 * fields that "Select League(s)" and the sort read, and +0x17.
 */
typedef struct {
    nation_t *nation;        /* +0x00 NULL for entries 0..7 */
    continent_t *continent;  /* +0x04 set for entries 3..7 */
    u32 setup;               /* +0x08 address of the function of the original
                                that sets the league up (inference). Not 0
                                for every entry. The port only tests it */
    s32 two_years;           /* +0x17 1 when a season spans two years
                                (inference from "Select Start Season") */
} league_t;

extern league_t g_leagues[LEAGUE_COUNT];

/*
 * FUN_00667150 for the fields above. The original also writes club comp
 * pointers (+0x1e, +0x22) and three dates (+0x15, +0x2a, +0x39), which need
 * the club comp table and the date code. The port leaves them out. A key id
 * of -1 gives a NULL nation or continent in the port, where the original
 * computes a pointer before the start of the table.
 */
bool league_table_build(void);

/*
 * FUN_0066a0c0. Sorts the rows 0..34 with the comparator 0x66a100 and
 * returns the static array (0xb63d38). Rows with neither nation nor
 * continent come first, in row order, then the continents by name, then the
 * nations by name, or by second name when `by_second_name` is not 0.
 */
const u8 *league_order(s32 by_second_name);

/*
 * FUN_00669f50. When `n` has an extra division, formats its text into `buf`
 * (if `buf` is not NULL) and returns 1. Returns 0 for any other nation, for
 * a NULL nation, and when `buf` is set and `size` < 20 (the original shows
 * an error box then).
 */
s32 league_extra_division(const nation_t *n, char *buf, s32 size);

#endif
