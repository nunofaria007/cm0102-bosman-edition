/*
 * Commands of the port driver for the database load
 * (tests/diff/cases_database.py).
 */
#include "port_database.h"
#include "database.h"
#include "key_index.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_LINKS 64

/* The count globals, in the order of cases_database.COUNTS. */
static void print_counts(void)
{
    const s32 values[] = {
        g_db_shared, g_db_index_header[0], g_db_index_header[1], g_continent_count,
        g_nation_count, g_city_count, g_stadium_count, g_club_count, g_nat_club_count,
        g_staff_count, g_non_player_count, g_player_count, g_preference_count,
        g_staff_history_count, g_club_comp_count, g_comp_history_count, g_nation_comp_count,
        g_nation_comp_history_count, g_staff_comp_count, g_staff_comp_history_count,
        g_official_count, g_colour_count, g_first_name_count, g_second_name_count,
        g_common_name_count,
    };

    for (size_t i = 0; i < sizeof(values) / sizeof(values[0]); i++) printf("%d ", values[i]);
    /* The rows where the three shared arrays change. */
    printf("%ld %ld %ld\n", g_nat_clubs ? (long)(g_nat_clubs - g_clubs) : -1L,
           g_nation_comps ? (long)(g_nation_comps - g_club_comps) : -1L,
           g_nation_comp_histories ? (long)(g_nation_comp_histories - g_comp_histories) : -1L);
}

bool database_command(const char *cmd, const char *args)
{
    char path[1024] = "";
    int table = 0;

    if (!strcmp(cmd, "db_fresh")) {
        /* The state of the original at its start: the key ids of the file, no
           cached name. */
        key_ids_initial();
        g_db_empty_common_name = -1;
    } else if (!strcmp(cmd, "db_read")) {
        sscanf(args, "%1023s", path);
        printf("%d\n", database_read(path));
    } else if (!strcmp(cmd, "db_resolve")) {
        database_resolve();
    } else if (!strcmp(cmd, "db_staff_pass")) {
        database_staff_pass();
        database_national_clubs();
    } else if (!strcmp(cmd, "db_keys")) {
        /* The key ids: the address of each global and its value. */
        key_index_set();
        for (s32 i = 0; i < KEY_ID_COUNT; i++) {
            printf("%u %d ", g_key_id_globals[i].address, g_key_ids[i]);
        }
        printf("\n");
    } else if (!strcmp(cmd, "db_named_keys")) {
        for (s32 k = 0; k < KEY_NATION_COUNT; k++) printf("%d ", g_key_nations[k]);
        for (s32 k = 0; k < KEY_CONTINENT_COUNT; k++) printf("%d ", g_key_continents[k]);
        printf("\n");
    } else if (!strcmp(cmd, "db_layouts")) {
        printf("%d\n", database_check_layouts());
    } else if (!strcmp(cmd, "db_free")) {
        database_free();
    } else if (!strcmp(cmd, "db_counts")) {
        print_counts();
    } else if (!strcmp(cmd, "db_info")) {
        db_link_t links[MAX_LINKS];
        s32 rows = 0, size = 0, n;

        sscanf(args, "%d", &table);
        n = database_table_info(table, &rows, &size, links, MAX_LINKS);
        printf("%d %d %d", rows, size, n);
        for (s32 i = 0; i < n && i < MAX_LINKS; i++) {
            printf(" %d %d %d", links[i].offset, links[i].count, links[i].target);
        }
        printf("\n");
    } else if (!strcmp(cmd, "db_dump")) {
        /* The size in bytes on one line, then the records. */
        db_link_t links[1];
        s32 rows = 0, size = 0;
        u8 *out;

        sscanf(args, "%d", &table);
        database_table_info(table, &rows, &size, links, 0);
        out = malloc((size_t)rows * (size_t)size + 1);
        if (!out) return false;
        for (s32 r = 0; r < rows; r++) database_record_bytes(table, r, out + (size_t)r * size);
        printf("%ld\n", (long)rows * size);
        fwrite(out, 1, (size_t)rows * size, stdout);
        free(out);
    } else {
        return false;
    }
    return true;
}
