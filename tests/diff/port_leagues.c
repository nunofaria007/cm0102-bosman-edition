/*
 * Commands of the port driver for the nation and league tables
 * (tests/diff/cases_leagues.py): index.dat, the nation and continent tables,
 * the key ids, the league table, the default selection, the league order,
 * the extra division text and the name compare.
 */
#include "port_leagues.h"
#include "database.h"
#include "exe_image.h"
#include "key_index.h"
#include "key_nation.h"
#include "langlib.h"
#include "port_helpers.h"
#include "setup.h"
#include "utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void print_bytes(const u8 *bytes, size_t size)
{
    for (size_t i = 0; i < size; i++) printf("%02x", bytes[i]);
    printf("\n");
}

static long row_of(const nation_t *n)
{
    return n ? (long)(n - g_nations) : -1;
}

static long continent_row(const continent_t *c)
{
    return c ? (long)(c - g_continents) : -1;
}

/* The flag byte of every nation, then the three counters. */
static void print_flags(void)
{
    for (s32 i = 0; i < g_nation_count; i++) printf("%02x", g_nations[i].league_flags);
    printf(" %d %d %ld\n", g_leagues_to_load, g_selected_nations, row_of(g_first_selected));
}

bool leagues_command(const char *cmd, const char *args)
{
    if (!strcmp(cmd, "lg_load")) {
        char path[1024] = "";
        sscanf(args, "%1023s", path);
        printf("%d %d %d\n", database_load_nations(path), g_continent_count, g_nation_count);
    } else if (!strcmp(cmd, "lg_free")) {
        database_free();
    } else if (!strcmp(cmd, "lg_index")) {
        printf("%d %d", g_db_index_header[0], g_db_index_header[1]);
        for (s32 i = 0; i < DB_INDEX_ENTRIES; i++) {
            const db_index_entry_t *e = &g_db_index[i];
            printf(" ");
            print_hex(e->name);
            printf(" %d %d %d %d", e->table, e->count, e->offset, e->format);
        }
        printf("\n");
    } else if (!strcmp(cmd, "lg_nation")) {
        u8 bytes[DB_NATION_SIZE];
        long row = next_long(&args);
        database_nation_bytes(&g_nations[row], bytes);
        print_bytes(bytes, sizeof(bytes));
    } else if (!strcmp(cmd, "lg_continent")) {
        u8 bytes[DB_CONTINENT_SIZE];
        long row = next_long(&args);
        database_continent_bytes(&g_continents[row], bytes);
        print_bytes(bytes, sizeof(bytes));
    } else if (!strcmp(cmd, "lg_rename")) {
        long row = next_long(&args);
        const char *name = next_text(&args);
        memset(g_nations[row].name, 0, sizeof(g_nations[row].name));
        strncpy(g_nations[row].name, name, sizeof(g_nations[row].name) - 1);
    } else if (!strcmp(cmd, "lg_rename2")) {
        long row = next_long(&args);
        const char *name = next_text(&args);
        memset(g_nations[row].second_name, 0, sizeof(g_nations[row].second_name));
        strncpy(g_nations[row].second_name, name, sizeof(g_nations[row].second_name) - 1);
    } else if (!strcmp(cmd, "lg_crename")) {
        long row = next_long(&args);
        const char *name = next_text(&args);
        memset(g_continents[row].name, 0, sizeof(g_continents[row].name));
        strncpy(g_continents[row].name, name, sizeof(g_continents[row].name) - 1);
    } else if (!strcmp(cmd, "lg_keys")) {
        /* The args are the path of cm0102.exe, which holds the key id names. */
        char path[1024] = "", error[512] = "";

        sscanf(args, "%1023s", path);
        if (!exe_image_loaded() && !exe_image_load(path, error, sizeof(error))) {
            fprintf(stderr, "lg_keys: %s\n", error);
            exit(1);
        }
        key_index_match();
        for (s32 i = 0; i < KEY_NATION_COUNT; i++) printf("%d ", g_key_nations[i]);
        for (s32 i = 0; i < KEY_CONTINENT_COUNT; i++) printf("%d ", g_key_continents[i]);
        printf("\n");
    } else if (!strcmp(cmd, "lg_table")) {
        printf("%d", league_table_build());
        for (s32 i = 0; i < LEAGUE_COUNT; i++) {
            const league_t *l = &g_leagues[i];
            printf(" %ld %ld %u %d", row_of(l->nation), continent_row(l->continent), l->setup,
                   l->two_years);
        }
        printf("\n");
    } else if (!strcmp(cmd, "lg_language")) {
        g_language_index = (s32)next_long(&args);
    } else if (!strcmp(cmd, "lg_set_flag")) {
        long row = next_long(&args);
        g_nations[row].league_flags = (u8)next_long(&args);
    } else if (!strcmp(cmd, "lg_counters")) {
        long first;
        g_leagues_to_load = (s32)next_long(&args);
        g_selected_nations = (s32)next_long(&args);
        first = next_long(&args);
        g_first_selected = first < 0 ? NULL : &g_nations[first];
    } else if (!strcmp(cmd, "lg_default")) {
        printf("%d ", setup_default_leagues());
        print_flags();
    } else if (!strcmp(cmd, "lg_order")) {
        const u8 *order = league_order((s32)next_long(&args));
        for (s32 i = 0; i < LEAGUE_COUNT; i++) printf("%d ", order[i]);
        printf("\n");
    } else if (!strcmp(cmd, "lg_extra")) {
        static char buf[TEXT_SIZE];
        long row = next_long(&args);
        long size = next_long(&args);
        long with_buffer = next_long(&args);
        s32 result;
        memset(buf, 0, sizeof(buf));
        result = league_extra_division(row < 0 ? NULL : &g_nations[row], with_buffer ? buf : NULL,
                                       (s32)size);
        printf("%d ", result);
        print_hex(buf);
        printf("\n");
    } else if (!strcmp(cmd, "lg_cmp")) {
        long keep_case = next_long(&args);
        long max_len = next_long(&args);
        const char *a = next_text(&args);
        const char *b = next_text(&args);
        printf("%d\n", utils_name_compare(a, b, (s32)keep_case, (u32)max_len));
    } else {
        return false;
    }
    return true;
}
