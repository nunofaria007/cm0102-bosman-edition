/*
 * Commands of the port driver for "Start New Game" and "Select League(s)"
 * (tests/diff/cases_select.py), on g_screen_stack with the screen manager of
 * the test scene and the tables of port_leagues.c.
 */
#include "port_select.h"
#include "database.h"
#include "key_index.h"
#include "port_helpers.h"
#include "port_scene.h"
#include "screen_stack.h"
#include "setup.h"
#include <stdio.h>
#include <string.h>

static int g_pumps;

/* Stands in for the message pump, which the test replaces in the original. */
static bool count_pump(void)
{
    g_pumps++;
    return true;
}

bool select_command(const char *cmd, const char *args)
{
    scrman_t *sm = g_screen_stack.record.screen_manager;

    if (!strcmp(cmd, "sl_new_game")) {
        printf("%d\n", setup_start_new_game());
    } else if (!strcmp(cmd, "sl_clear_keys")) {
        /* The load must find the key ids again. */
        for (s32 k = 0; k < KEY_NATION_COUNT; k++) g_key_nations[k] = -1;
        for (s32 k = 0; k < KEY_CONTINENT_COUNT; k++) g_key_continents[k] = -1;
    } else if (!strcmp(cmd, "sl_record")) {
        /* Flips and pumps from here on. */
        g_screen_pump = count_pump;
        g_pumps = 0;
        scene_record_flips();
    } else if (!strcmp(cmd, "sl_pumps")) {
        printf("%d\n", g_pumps);
        g_pumps = 0;
    } else if (!strcmp(cmd, "sl_options")) {
        g_real_players = (u8)next_long(&args);
        g_attribute_masking = (u8)next_long(&args);
    } else if (!strcmp(cmd, "sl_state")) {
        /* The flag byte of every nation, the counters and the options. */
        for (s32 i = 0; i < g_nation_count; i++) printf("%02x", g_nations[i].league_flags);
        printf(" %d %d %ld %d %d\n", g_leagues_to_load, g_selected_nations,
               g_first_selected ? (long)(g_first_selected - g_nations) : -1L,
               g_real_players, g_attribute_masking);
    } else if (!strcmp(cmd, "sl_data")) {
        /* The data of a widget: "n" and the row of a nation, else the value. */
        intptr_t data = sm->widgets[next_long(&args)].unknown_48;
        intptr_t first = (intptr_t)g_nations;

        if (g_nations && data >= first && data < (intptr_t)(g_nations + g_nation_count) &&
            (data - first) % (intptr_t)sizeof(nation_t) == 0) {
            printf("n%ld\n", (long)((data - first) / (intptr_t)sizeof(nation_t)));
        } else {
            printf("%u\n", (u32)data);
        }
    } else {
        return false;
    }
    return true;
}
