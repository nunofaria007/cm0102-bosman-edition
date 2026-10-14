#include "key_index.h"
#include <string.h>

/*
 * Key ids of index.cpp. See key_index.h. FUN_0060d810 is an if-chain over
 * 209 names, and the first name that matches sets its global. The 27 names
 * here are each once in the chain, so the first match is the only match.
 */

s32 g_key_nations[KEY_NATION_COUNT];
s32 g_key_continents[KEY_CONTINENT_COUNT];

/* The strings of the chain, from the writes at 0x60d8da (Argentina) to 0x60ee33 (Wales). */
const char *const g_key_nation_names[KEY_NATION_COUNT] = {
    "Argentina", "Australia", "Belgium", "Brazil", "Croatia", "Denmark",
    "England", "Finland", "France", "Germany", "Greece", "Holland",
    "Republic Of Ireland", "Italy", "Japan", "Northern Ireland", "Norway",
    "Poland", "Portugal", "Russia", "Scotland", "South Korea", "Spain",
    "Sweden", "Turkey", "United States", "Wales",
};

/* 0x9d9458 .. 0x9d9420, in the order of the chain at 0x604289. */
const char *const g_key_continent_names[KEY_CONTINENT_COUNT] = {
    "Africa", "Asia", "Europe", "North America", "Oceania", "South America",
};

/*
 * FUN_00951c50, _stricmp of the C locale (the locale word at 0xde3048 is
 * 0): equal when the bytes are equal after A..Z become a..z. Only the
 * equality matters here.
 */
static bool same_name(const char *a, const char *b)
{
    for (;; a++, b++) {
        u8 x = (u8)*a, y = (u8)*b;

        if (x >= 'A' && x <= 'Z') x += 0x20;
        if (y >= 'A' && y <= 'Z') y += 0x20;
        if (x != y) return false;
        if (x == 0) return true;
    }
}

void key_index_match(void)
{
    for (s32 k = 0; k < KEY_NATION_COUNT; k++) g_key_nations[k] = -1;
    for (s32 k = 0; k < KEY_CONTINENT_COUNT; k++) g_key_continents[k] = -1;
    /* 0x60420d: FUN_0060d810(nation + 4, row) for each row. */
    for (s32 row = 0; row < g_nation_count; row++) {
        for (s32 k = 0; k < KEY_NATION_COUNT; k++) {
            if (same_name(g_nations[row].name, g_key_nation_names[k])) {
                g_key_nations[k] = row;
                break;
            }
        }
    }
    /* 0x604278..0x604347. */
    for (s32 row = 0; row < g_continent_count; row++) {
        for (s32 k = 0; k < KEY_CONTINENT_COUNT; k++) {
            if (same_name(g_continents[row].name, g_key_continent_names[k])) {
                g_key_continents[k] = row;
                break;
            }
        }
    }
}

nation_t *key_nation(enum key_nation key)
{
    s32 row = g_key_nations[key];
    return row < 0 || row >= g_nation_count ? NULL : &g_nations[row];
}

continent_t *key_continent(enum key_continent key)
{
    s32 row = g_key_continents[key];
    return row < 0 || row >= g_continent_count ? NULL : &g_continents[row];
}
