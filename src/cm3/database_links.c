#include "database.h"

/*
 * The id pass FUN_005315a0, the staff pass and the national clubs of
 * FUN_00528540. This first part links only the continent and the rivals of
 * each nation, which the league table needs.
 */

s32 g_db_empty_common_name = -1;

void database_resolve(void)
{
    for (s32 i = 0; i < g_nation_count; i++) {
        nation_t *n = &g_nations[i];
        s32 id = DB_ID(n->continent);

        n->continent = id < 0 || id >= g_continent_count ? NULL : &g_continents[id];
        for (s32 k = 0; k < 3; k++) {
            id = DB_ID(n->rivals[k]);
            n->rivals[k] = id < 0 || id >= g_nation_count ? NULL : &g_nations[id];
        }
    }
}

void database_staff_pass(void)
{
}

void database_national_clubs(void)
{
}
