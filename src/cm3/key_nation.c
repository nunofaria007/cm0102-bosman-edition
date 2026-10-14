#include "key_nation.h"
#include "key_index.h"
#include "langlib.h"
#include "utils.h"
#include <stdlib.h>
#include <string.h>

/*
 * The league table of key_nation.cpp. See key_nation.h.
 */

#define NO_KEY (-1)

league_t g_leagues[LEAGUE_COUNT];

static u8 g_league_order[LEAGUE_COUNT];   /* 0xb63d38 */
static s32 g_order_by_second_name;        /* DAT_00b63d5c */

/*
 * What FUN_00667150 writes into each entry: the key nation, the key
 * continent, the function pointer (+0x08) and the word at +0x17. Entries
 * 0..2 have neither nation nor continent (inference: world competitions).
 */
static const struct {
    s32 nation;      /* enum key_nation, or NO_KEY */
    s32 continent;   /* enum key_continent, or NO_KEY */
    u32 setup;
    s32 two_years;
} g_league_data[LEAGUE_COUNT] = {
    {NO_KEY, NO_KEY, 0x830620, 1},                       /* 0x667219 */
    {NO_KEY, NO_KEY, 0x830ac0, 1},                       /* 0x6672a7 */
    {NO_KEY, NO_KEY, 0x830fe0, 1},                       /* 0x667344 */
    {NO_KEY, KEY_EUROPE, 0x8312e0, 1},                   /* 0x6673d6 */
    {NO_KEY, KEY_NORTH_AMERICA, 0x831650, 1},            /* 0x6674a8 */
    {NO_KEY, KEY_SOUTH_AMERICA, 0x8318f0, 1},
    {NO_KEY, KEY_ASIA, 0x831ba0, 1},
    {NO_KEY, KEY_OCEANIA, 0x831eb0, 1},
    {KEY_ARGENTINA, NO_KEY, 0x832100, 1},                /* 0x667887 */
    {KEY_AUSTRALIA, NO_KEY, 0x832380, 1},
    {KEY_BELGIUM, NO_KEY, 0x8325d0, 1},
    {KEY_BRAZIL, NO_KEY, 0x832980, 0},
    {KEY_CROATIA, NO_KEY, 0x8330b0, 1},
    {KEY_DENMARK, NO_KEY, 0x833400, 1},
    {KEY_ENGLAND, NO_KEY, 0x833750, 1},
    {KEY_FINLAND, NO_KEY, 0x833cf0, 0},
    {KEY_FRANCE, NO_KEY, 0x833ff0, 1},
    {KEY_GERMANY, NO_KEY, 0x8343f0, 1},
    {KEY_GREECE, NO_KEY, 0x8347c0, 1},
    {KEY_HOLLAND, NO_KEY, 0x834b10, 1},
    {KEY_IRELAND, NO_KEY, 0x834e60, 1},
    {KEY_ITALY, NO_KEY, 0x835320, 1},
    {KEY_JAPAN, NO_KEY, 0x835900, 0},
    {KEY_NORTHERN_IRELAND, NO_KEY, 0x835c90, 1},
    {KEY_NORWAY, NO_KEY, 0x836050, 0},
    {KEY_POLAND, NO_KEY, 0x836350, 1},
    {KEY_PORTUGAL, NO_KEY, 0x8366e0, 1},
    {KEY_RUSSIA, NO_KEY, 0x836ac0, 0},
    {KEY_SCOTLAND, NO_KEY, 0x836dc0, 1},
    {KEY_SOUTH_KOREA, NO_KEY, 0x837200, 0},
    {KEY_SPAIN, NO_KEY, 0x837560, 1},
    {KEY_SWEDEN, NO_KEY, 0x8379f0, 0},
    {KEY_TURKEY, NO_KEY, 0x837d70, 1},
    {KEY_UNITED_STATES, NO_KEY, 0x8380c0, 0},
    {KEY_WALES, NO_KEY, 0x8383b0, 1},                    /* 0x669047 */
};

bool league_table_build(void)
{
    /* 0x667175: "key_nation.cpp" line 0x36 error box when no nation table. */
    if (!g_nations) return false;
    memset(g_leagues, 0, sizeof(g_leagues));
    for (s32 i = 0; i < LEAGUE_COUNT; i++) {
        league_t *l = &g_leagues[i];

        if (g_league_data[i].nation != NO_KEY) l->nation = key_nation(g_league_data[i].nation);
        if (g_league_data[i].continent != NO_KEY) {
            l->continent = key_continent(g_league_data[i].continent);
        }
        l->setup = g_league_data[i].setup;
        l->two_years = g_league_data[i].two_years;
    }
    return true;
}

/*
 * The comparator 0x66a100. Each element is one signed byte, a row of
 * g_leagues. A NULL element pointer, which qsort never passes, sorts last.
 */
static int compare_leagues(const void *pa, const void *pb)
{
    s8 a, b;
    const nation_t *an, *bn;
    const continent_t *ac, *bc;

    if (!pa) return 1;
    if (!pb) return -1;
    a = *(const s8 *)pa;
    b = *(const s8 *)pb;
    an = g_leagues[a].nation;
    ac = g_leagues[a].continent;
    bn = g_leagues[b].nation;
    bc = g_leagues[b].continent;
    if (!an && !ac) {
        if (bn || bc) return -1;
        /* 0x66a191: rows 0, 1 and 2 in their order. */
        if (a == 0) return b == 1 || b == 2 ? -1 : 0;
        if (a == 1) return b == 2 ? -1 : b == 0 ? 1 : 0;
        if (a == 2) return b == 0 || b == 1 ? 1 : 0;
        return 0;
    }
    if (!bn && !bc) return 1;
    if (!an || ac) {
        /* 0x66a1ff: a has a continent. */
        if (!bc) return -1;
        return utils_name_compare(ac->name, bc->name, 1, 0);
    }
    /* 0x66a20e: a has a nation and no continent. */
    if (bc) return 1;
    if (!bn) return 0;
    if (g_order_by_second_name) {
        return utils_name_compare(an->second_name, bn->second_name, 1, 0);
    }
    return utils_name_compare(an->name, bn->name, 1, 0);
}

/*
 * The original sorts with the qsort of MSVC (FUN_00945333). The comparator
 * gives no two rows as equal unless two key records share a name, so the
 * host qsort gives the same order.
 */
const u8 *league_order(s32 by_second_name)
{
    for (s32 i = 0; i < LEAGUE_COUNT; i++) g_league_order[i] = (u8)i;
    g_order_by_second_name = by_second_name;
    qsort(g_league_order, LEAGUE_COUNT, 1, compare_leagues);
    return g_league_order;
}

s32 league_extra_division(const nation_t *n, char *buf, s32 size)
{
    const char *format = NULL;
    s32 id;

    if (!n) return 0;
    /* 0x669f69: "key_nation.cpp" line 0x4a1 error box. */
    if (buf && size < 20) return 0;
    id = n->id;
    if (id == g_key_nations[KEY_ENGLAND]) {
        format = "Conference Division<%s - COMMENT - English Conference>";   /* 0x9eb078 */
    } else if (id == g_key_nations[KEY_GERMANY]) {
        format = "Regional Divisions";                                       /* 0x9eb064 */
    } else if (id == g_key_nations[KEY_ITALY]) {
        format = "Serie C2 A, B, C";                                         /* 0x9eb050 */
    } else if (id == g_key_nations[KEY_PORTUGAL] || id == g_key_nations[KEY_SPAIN]) {
        format = "Second Division B";                                        /* 0x9eb03c */
    } else if (id == g_key_nations[KEY_SWEDEN]) {
        format = "Second Division";                                          /* 0x9eb02c */
    } else {
        return 0;
    }
    /*
     * The original passes no size to FUN_0066f4e0. The port uses `size`,
     * which is at least 20 here.
     */
    if (buf) {
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wformat-nonliteral"
        text_format(buf, (size_t)size, format, "");
#pragma clang diagnostic pop
    }
    return 1;
}
