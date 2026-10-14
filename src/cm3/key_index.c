#include "key_index.h"
#include <stdlib.h>
#include <string.h>

/*
 * Key ids of index.cpp. See key_index.h. The chains are in
 * key_ids_table.c.
 */

s32 g_key_ids[KEY_ID_COUNT] = {[0 ... KEY_ID_COUNT - 1] = -1};
s32 g_key_nations[KEY_NATION_COUNT] = {[0 ... KEY_NATION_COUNT - 1] = -1};
s32 g_key_continents[KEY_CONTINENT_COUNT] = {[0 ... KEY_CONTINENT_COUNT - 1] = -1};

/* The globals of the enums. */
static const u32 nation_globals[KEY_NATION_COUNT] = {
    0x9cf218, 0x9cf224, 0x9cf244, 0x9cf260, 0x9cf2ac, 0x9cf2c0, 0x9cf2e4, 0x9cf2fc, 0x9cf300,
    0x9cf314, 0x9cf31c, 0x9cf338, 0x9cf2dc, 0x9cf360, 0x9cf36c, 0x9cf3e4, 0x9cf408, 0x9cf430,
    0x9cf434, 0x9cf448, 0x9cf45c, 0x9cf484, 0x9cf488, 0x9cf4a8, 0x9cf4dc, 0x9cf4f8, 0x9cf510,
};

static const u32 continent_globals[KEY_CONTINENT_COUNT] = {
    0x9cfa08, 0x9cfa0c, 0x9cfa10, 0x9cfa14, 0x9cfa18, 0x9cfa1c,
};

static int compare_global(const void *key, const void *entry)
{
    u32 a = *(const u32 *)key, b = ((const key_id_global_t *)entry)->address;
    return a < b ? -1 : a > b;
}

s32 *key_id(u32 address)
{
    const key_id_global_t *g = bsearch(&address, g_key_id_globals, KEY_ID_COUNT,
                                       sizeof(g_key_id_globals[0]), compare_global);
    return g ? &g_key_ids[g - g_key_id_globals] : NULL;
}

void key_ids_initial(void)
{
    for (s32 i = 0; i < KEY_ID_COUNT; i++) g_key_ids[i] = g_key_id_globals[i].initial;
    for (s32 k = 0; k < KEY_NATION_COUNT; k++) g_key_nations[k] = *key_id(nation_globals[k]);
    for (s32 k = 0; k < KEY_CONTINENT_COUNT; k++) g_key_continents[k] = *key_id(continent_globals[k]);
}

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

/*
 * One row through a function of chains: in each chain the first equal name
 * stores the row in its key ids.
 */
static void match(const key_id_entry_t *chain, s32 length, const char *name, s32 row)
{
    s32 done = -1;

    for (s32 i = 0; i < length; i++) {
        const key_id_entry_t *e = &chain[i];

        if (e->chain == done || !same_name(name, e->name)) continue;
        g_key_ids[e->slot] = row;
        if (e->slot2 >= 0) g_key_ids[e->slot2] = row;
        done = e->chain;
    }
}

/* The rows of a table, by their name at +0x04. */
#define MATCH_ROWS(chain, rows, count)                                        \
    for (s32 row_ = 0; row_ < (count); row_++)                                \
        match(chain, chain##_length, (rows)[row_].name, row_)

/* The arrays of the enums from g_key_ids. */
static void copy_named(void)
{
    for (s32 k = 0; k < KEY_NATION_COUNT; k++) g_key_nations[k] = *key_id(nation_globals[k]);
    for (s32 k = 0; k < KEY_CONTINENT_COUNT; k++) g_key_continents[k] = *key_id(continent_globals[k]);
}

void key_index_match(void)
{
    for (s32 k = 0; k < KEY_NATION_COUNT; k++) *key_id(nation_globals[k]) = -1;
    for (s32 k = 0; k < KEY_CONTINENT_COUNT; k++) *key_id(continent_globals[k]) = -1;
    MATCH_ROWS(g_key_nation_chain, g_nations, g_nation_count);         /* 0x60420d */
    MATCH_ROWS(g_key_continent_chain, g_continents, g_continent_count); /* 0x604278 */
    copy_named();
}

/* FUN_00601a40 */
void key_index_set(void)
{
    for (s32 i = 0; i < KEY_ID_COUNT; i++) {                         /* 0x601a4a */
        if (g_key_id_globals[i].reset) g_key_ids[i] = -1;
    }
    MATCH_ROWS(g_key_colour_chain, g_colours, g_colour_count);         /* 0x60386c */
    MATCH_ROWS(g_key_nation_chain, g_nations, g_nation_count);         /* 0x60420d */
    MATCH_ROWS(g_key_club_comp_chain, g_club_comps, g_club_comp_count - 127);  /* 0x604241 */
    MATCH_ROWS(g_key_continent_chain, g_continents, g_continent_count); /* 0x604278 */
    MATCH_ROWS(g_key_staff_comp_chain, g_staff_comps, g_staff_comp_count);     /* 0x604347 */
    MATCH_ROWS(g_key_club_chain, g_clubs, g_club_count);               /* 0x604378 */
    copy_named();
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
