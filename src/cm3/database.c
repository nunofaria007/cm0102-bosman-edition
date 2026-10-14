#include "database.h"
#include "key_index.h"
#include "langlib.h"
#include "progress_box.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

extern char *os_find_file_ci(const char *dir, const char *name);

/*
 * The loader of Database.cpp and the readers of Db_files.cpp. See
 * database.h. The id pass and the staff pass are in database_links.c.
 */

/* The load makes 47 calls of the progress box, steps 0 to 46. */
#define DB_PROGRESS_TOTAL 46
/* Staff and player rows that the load allocates but does not count. */
#define DB_SPARE_ROWS 0x52d
#define DB_RESERVE_STAFF 23000    /* 0x59d8 */
#define DB_RESERVE_COMPS 127      /* 0x7f */
#define DB_MANAGER_ROWS 16

s32 g_db_index_header[2];
db_index_entry_t g_db_index[DB_INDEX_ENTRIES];
s32 g_db_shared;

club_t *g_clubs;
s32 g_club_count;
club_t *g_nat_clubs;
s32 g_nat_club_count;
colour_t *g_colours;
s32 g_colour_count;
continent_t *g_continents;
s32 g_continent_count;
nation_t *g_nations;
s32 g_nation_count;
stadium_t *g_stadiums;
s32 g_stadium_count;
staff_t *g_staff;
s32 g_staff_count;
official_t *g_officials;
s32 g_official_count;
non_player_t *g_non_players;
s32 g_non_player_count;
player_t *g_players;
s32 g_player_count;
staff_comp_t *g_staff_comps;
s32 g_staff_comp_count;
club_comp_t *g_club_comps;
s32 g_club_comp_count;
club_comp_t *g_nation_comps;
s32 g_nation_comp_count;
db_name_t *g_first_names;
s32 g_first_name_count;
db_name_t *g_second_names;
s32 g_second_name_count;
db_name_t *g_common_names;
s32 g_common_name_count;
staff_history_t *g_staff_histories;
s32 g_staff_history_count;
staff_comp_history_t *g_staff_comp_histories;
s32 g_staff_comp_history_count;
comp_history_t *g_comp_histories;
s32 g_comp_history_count;
comp_history_t *g_nation_comp_histories;
s32 g_nation_comp_history_count;
city_t *g_cities;
s32 g_city_count;
preferences_t *g_preferences;
s32 g_preference_count;

/* The 157-byte staff records of the file (DAT_00ae2404, count DAT_00ae2400). */
static u8 *g_staff_file;
static s32 g_staff_file_count;

/*
 * Which pointer fields hold pointers: none before the id pass, the
 * continent and the rivals of each nation after database_load_nations, all
 * after the id pass.
 */
enum { LINKS_NONE, LINKS_NATIONS, LINKS_ALL };
static s32 g_links;

/* Record layouts */

/*
 * One field of a record: its offset and size in the file, its offset in
 * the struct, and for a pointer field the table it points into and the
 * number of pointers.
 */
typedef struct {
    u16 file;
    u16 size;
    u16 native;
    s8 target;       /* -1: bytes */
    u8 count;
    u8 nation_link;  /* linked by database_load_nations */
} db_field_t;

#define RAW(T, off, f) {off, sizeof(((T *)0)->f), offsetof(T, f), -1, 1, 0}
#define PTRS(T, off, f, table)                                                     \
    {off, sizeof(((T *)0)->f) / sizeof(void *) * 4, offsetof(T, f), table,         \
     sizeof(((T *)0)->f) / sizeof(void *), 0}
#define PTR_EARLY(T, off, f, table)                                                \
    {off, sizeof(((T *)0)->f) / sizeof(void *) * 4, offsetof(T, f), table,         \
     sizeof(((T *)0)->f) / sizeof(void *), 1}

static const db_field_t continent_fields[] = {
    RAW(continent_t, 0x00, id), RAW(continent_t, 0x04, name), RAW(continent_t, 0x1e, unknown_1e),
    RAW(continent_t, 0x1f, code), RAW(continent_t, 0x23, adjective),
    RAW(continent_t, 0x3d, long_name), RAW(continent_t, 0xa2, unknown_a2),
    RAW(continent_t, 0xa3, short_code), RAW(continent_t, 0xbd, unknown_bd),
    RAW(continent_t, 0xbe, unknown_be),
};

static const db_field_t nation_fields[] = {
    RAW(nation_t, 0x000, id), RAW(nation_t, 0x004, name), RAW(nation_t, 0x037, unknown_037),
    RAW(nation_t, 0x038, second_name), RAW(nation_t, 0x052, unknown_052),
    RAW(nation_t, 0x053, code), RAW(nation_t, 0x057, nationality),
    PTR_EARLY(nation_t, 0x071, continent, DB_CONTINENT), RAW(nation_t, 0x075, unknown_075),
    PTRS(nation_t, 0x07a, city, DB_CITY), RAW(nation_t, 0x07e, unknown_07e),
    PTRS(nation_t, 0x080, stadium, DB_STADIUM), RAW(nation_t, 0x084, unknown_084),
    RAW(nation_t, 0x08e, unknown_08e), PTRS(nation_t, 0x090, colours, DB_COLOUR),
    RAW(nation_t, 0x0a8, unknown_0a8), PTR_EARLY(nation_t, 0x110, rivals, DB_NATION),
    RAW(nation_t, 0x11c, league_flags), RAW(nation_t, 0x11d, unknown_11d),
};

static const db_field_t club_fields[] = {
    RAW(club_t, 0x000, id), RAW(club_t, 0x004, name), RAW(club_t, 0x037, unknown_037),
    RAW(club_t, 0x038, short_name), RAW(club_t, 0x052, unknown_052),
    PTRS(club_t, 0x053, nation, DB_NATION), PTRS(club_t, 0x057, division, DB_CLUB_COMP),
    PTRS(club_t, 0x05b, last_division, DB_CLUB_COMP), RAW(club_t, 0x05f, unknown_05f),
    PTRS(club_t, 0x060, comp_060, DB_CLUB_COMP), RAW(club_t, 0x064, unknown_064),
    PTRS(club_t, 0x069, stadium, DB_STADIUM), RAW(club_t, 0x06d, unknown_06d),
    PTRS(club_t, 0x06e, stadium_06e, DB_STADIUM), RAW(club_t, 0x072, unknown_072),
    RAW(club_t, 0x073, unknown_073), RAW(club_t, 0x077, unknown_077),
    RAW(club_t, 0x07b, unknown_07b), RAW(club_t, 0x07f, unknown_07f),
    RAW(club_t, 0x080, reputation), RAW(club_t, 0x082, unknown_082),
    PTRS(club_t, 0x083, colours, DB_COLOUR), PTRS(club_t, 0x09b, staff_09b, DB_STAFF),
    PTRS(club_t, 0x0b3, clubs_0b3, DB_CLUB), PTRS(club_t, 0x0bf, chairman, DB_STAFF),
    PTRS(club_t, 0x0c3, directors, DB_STAFF), PTRS(club_t, 0x0cf, manager, DB_STAFF),
    PTRS(club_t, 0x0d3, assistant, DB_STAFF), PTRS(club_t, 0x0d7, squad, DB_STAFF),
    PTRS(club_t, 0x19f, coaches, DB_STAFF), PTRS(club_t, 0x1b3, scouts, DB_STAFF),
    PTRS(club_t, 0x1cf, physios, DB_STAFF), RAW(club_t, 0x1db, unknown_1db),
    RAW(club_t, 0x1df, unknown_1df), PTRS(club_t, 0x1e0, team, DB_STAFF),
    RAW(club_t, 0x230, unknown_230), RAW(club_t, 0x244, in_league),
};

static const db_field_t colour_fields[] = {
    RAW(colour_t, 0x00, id), RAW(colour_t, 0x04, name), RAW(colour_t, 0x37, unknown_37),
};

static const db_field_t stadium_fields[] = {
    RAW(stadium_t, 0x00, id), RAW(stadium_t, 0x04, name), RAW(stadium_t, 0x37, unknown_37),
    PTRS(stadium_t, 0x38, city, DB_CITY), RAW(stadium_t, 0x3c, unknown_3c),
    PTRS(stadium_t, 0x48, stadium_48, DB_STADIUM), RAW(stadium_t, 0x4c, unknown_4c),
};

static const db_field_t staff_fields[] = {
    RAW(staff_t, 0x00, id), PTRS(staff_t, 0x04, first_name, DB_FIRST_NAME),
    PTRS(staff_t, 0x08, second_name, DB_SECOND_NAME),
    PTRS(staff_t, 0x0c, common_name, DB_COMMON_NAME), RAW(staff_t, 0x10, born),
    RAW(staff_t, 0x18, age), PTRS(staff_t, 0x1a, nation, DB_NATION),
    PTRS(staff_t, 0x1e, nation2, DB_NATION), RAW(staff_t, 0x22, b22), RAW(staff_t, 0x23, b23),
    PTRS(staff_t, 0x24, national_club, DB_CLUB), RAW(staff_t, 0x28, national_job),
    RAW(staff_t, 0x29, d29), RAW(staff_t, 0x31, d31), PTRS(staff_t, 0x39, club, DB_CLUB),
    RAW(staff_t, 0x3d, club_job), RAW(staff_t, 0x3e, contract_end), RAW(staff_t, 0x46, d46),
    RAW(staff_t, 0x4e, i4e), RAW(staff_t, 0x52, i52), RAW(staff_t, 0x56, unknown_56),
    RAW(staff_t, 0x5e, b5e), RAW(staff_t, 0x5f, b5f), RAW(staff_t, 0x60, b60),
    PTRS(staff_t, 0x61, player, DB_PLAYER), PTRS(staff_t, 0x65, prefs, DB_PREFERENCES),
    PTRS(staff_t, 0x69, non_player, DB_NON_PLAYER), RAW(staff_t, 0x6d, b6d),
};

static const db_field_t official_fields[] = {
    RAW(official_t, 0x00, id), PTRS(official_t, 0x04, first_name, DB_FIRST_NAME),
    PTRS(official_t, 0x08, second_name, DB_SECOND_NAME), RAW(official_t, 0x0c, unknown_0c),
    PTRS(official_t, 0x16, nation, DB_NATION), PTRS(official_t, 0x1a, city, DB_CITY),
    RAW(official_t, 0x1e, unknown_1e),
};

static const db_field_t non_player_fields[] = {
    RAW(non_player_t, 0x00, id), RAW(non_player_t, 0x04, unknown_04),
    RAW(non_player_t, 0x0e, unknown_0e), PTRS(non_player_t, 0x23, staff, DB_STAFF),
    RAW(non_player_t, 0x43, unknown_43),
};

static const db_field_t player_fields[] = {
    RAW(player_t, 0x00, id), RAW(player_t, 0x04, unknown_04), RAW(player_t, 0x05, unknown_05),
    RAW(player_t, 0x0f, unknown_0f),
};

static const db_field_t staff_comp_fields[] = {
    RAW(staff_comp_t, 0x00, id), RAW(staff_comp_t, 0x04, name), RAW(staff_comp_t, 0x37, unknown_37),
    PTRS(staff_comp_t, 0x53, continent, DB_CONTINENT), PTRS(staff_comp_t, 0x57, nation, DB_NATION),
    PTRS(staff_comp_t, 0x5b, colours, DB_COLOUR), RAW(staff_comp_t, 0x63, unknown_63),
};

static const db_field_t club_comp_fields[] = {
    RAW(club_comp_t, 0x00, id), RAW(club_comp_t, 0x04, name), RAW(club_comp_t, 0x37, unknown_37),
    RAW(club_comp_t, 0x38, short_name), RAW(club_comp_t, 0x52, unknown_52),
    RAW(club_comp_t, 0x53, b53), RAW(club_comp_t, 0x54, unknown_54), RAW(club_comp_t, 0x57, b57),
    RAW(club_comp_t, 0x58, b58), PTRS(club_comp_t, 0x59, continent, DB_CONTINENT),
    PTRS(club_comp_t, 0x5d, nation, DB_NATION), PTRS(club_comp_t, 0x61, colours, DB_COLOUR),
    RAW(club_comp_t, 0x69, level),
};

static const db_field_t name_fields[] = {
    RAW(db_name_t, 0x00, name), RAW(db_name_t, 0x33, id), RAW(db_name_t, 0x37, nation_id),
    RAW(db_name_t, 0x3b, count),
};

static const db_field_t staff_history_fields[] = {
    RAW(staff_history_t, 0x00, id), PTRS(staff_history_t, 0x04, staff, DB_STAFF),
    RAW(staff_history_t, 0x08, unknown_08), PTRS(staff_history_t, 0x0a, club, DB_CLUB),
    RAW(staff_history_t, 0x0e, unknown_0e),
};

static const db_field_t staff_comp_history_fields[] = {
    RAW(staff_comp_history_t, 0x00, id), RAW(staff_comp_history_t, 0x04, unknown_04),
};

static const db_field_t comp_history_fields[] = {
    RAW(comp_history_t, 0x00, id), PTRS(comp_history_t, 0x04, comp, DB_CLUB_COMP),
    RAW(comp_history_t, 0x08, unknown_08), PTRS(comp_history_t, 0x0a, clubs, DB_CLUB),
};

static const db_field_t city_fields[] = {
    RAW(city_t, 0x00, id), RAW(city_t, 0x04, name), RAW(city_t, 0x1e, unknown_1e),
    PTRS(city_t, 0x1f, nation, DB_NATION), RAW(city_t, 0x23, unknown_23),
};

static const db_field_t preferences_fields[] = {
    RAW(preferences_t, 0x00, id), PTRS(preferences_t, 0x04, clubs, DB_CLUB),
    PTRS(preferences_t, 0x1c, staff, DB_STAFF),
};

/* A table: its layout and its two globals. */
typedef struct {
    s32 size;              /* record size of the original */
    size_t native;         /* sizeof the struct */
    const db_field_t *fields;
    s32 field_count;
    void **base;
    s32 *count;
} db_layout_t;

#define LAYOUT(size, T, fields, base, count) \
    {size, sizeof(T), fields, sizeof(fields) / sizeof(fields[0]), (void **)&base, &count}

static const db_layout_t layouts[DB_TABLE_IDS] = {
    [DB_CLUB] = LAYOUT(DB_CLUB_SIZE, club_t, club_fields, g_clubs, g_club_count),
    [DB_NAT_CLUB] = LAYOUT(DB_CLUB_SIZE, club_t, club_fields, g_nat_clubs, g_nat_club_count),
    [DB_COLOUR] = LAYOUT(DB_COLOUR_SIZE, colour_t, colour_fields, g_colours, g_colour_count),
    [DB_CONTINENT] = LAYOUT(DB_CONTINENT_SIZE, continent_t, continent_fields, g_continents,
                            g_continent_count),
    [DB_NATION] = LAYOUT(DB_NATION_SIZE, nation_t, nation_fields, g_nations, g_nation_count),
    [DB_STADIUM] = LAYOUT(DB_STADIUM_SIZE, stadium_t, stadium_fields, g_stadiums, g_stadium_count),
    [DB_STAFF] = LAYOUT(DB_STAFF_SIZE, staff_t, staff_fields, g_staff, g_staff_count),
    [DB_OFFICIAL] = LAYOUT(DB_OFFICIAL_SIZE, official_t, official_fields, g_officials,
                           g_official_count),
    [DB_NON_PLAYER] = LAYOUT(DB_NON_PLAYER_SIZE, non_player_t, non_player_fields, g_non_players,
                             g_non_player_count),
    [DB_PLAYER] = LAYOUT(DB_PLAYER_SIZE, player_t, player_fields, g_players, g_player_count),
    [DB_STAFF_COMP] = LAYOUT(DB_STAFF_COMP_SIZE, staff_comp_t, staff_comp_fields, g_staff_comps,
                             g_staff_comp_count),
    [DB_CLUB_COMP] = LAYOUT(DB_CLUB_COMP_SIZE, club_comp_t, club_comp_fields, g_club_comps,
                            g_club_comp_count),
    [DB_FIRST_NAME] = LAYOUT(DB_NAME_SIZE, db_name_t, name_fields, g_first_names,
                             g_first_name_count),
    [DB_SECOND_NAME] = LAYOUT(DB_NAME_SIZE, db_name_t, name_fields, g_second_names,
                              g_second_name_count),
    [DB_COMMON_NAME] = LAYOUT(DB_NAME_SIZE, db_name_t, name_fields, g_common_names,
                              g_common_name_count),
    [DB_NATION_COMP] = LAYOUT(DB_CLUB_COMP_SIZE, club_comp_t, club_comp_fields, g_nation_comps,
                              g_nation_comp_count),
    [DB_STAFF_HISTORY] = LAYOUT(DB_STAFF_HISTORY_SIZE, staff_history_t, staff_history_fields,
                                g_staff_histories, g_staff_history_count),
    [DB_STAFF_COMP_HISTORY] = LAYOUT(DB_STAFF_COMP_HISTORY_SIZE, staff_comp_history_t,
                                     staff_comp_history_fields, g_staff_comp_histories,
                                     g_staff_comp_history_count),
    [DB_COMP_HISTORY] = LAYOUT(DB_COMP_HISTORY_SIZE, comp_history_t, comp_history_fields,
                               g_comp_histories, g_comp_history_count),
    [DB_NATION_COMP_HISTORY] = LAYOUT(DB_COMP_HISTORY_SIZE, comp_history_t, comp_history_fields,
                                      g_nation_comp_histories, g_nation_comp_history_count),
    [DB_CITY] = LAYOUT(DB_CITY_SIZE, city_t, city_fields, g_cities, g_city_count),
    [DB_PREFERENCES] = LAYOUT(DB_PREFERENCES_SIZE, preferences_t, preferences_fields,
                              g_preferences, g_preference_count),
};

static s32 get_s32(const u8 *p)
{
    return (s32)((u32)p[0] | (u32)p[1] << 8 | (u32)p[2] << 16 | (u32)p[3] << 24);
}

static void put_s32(u8 *p, s32 value)
{
    for (s32 i = 0; i < 4; i++) p[i] = (u8)((u32)value >> (8 * i));
}

/* One record from the bytes of the file. Each pointer field gets its id. */
static void record_from_bytes(const db_layout_t *layout, void *record, const u8 *bytes)
{
    for (s32 i = 0; i < layout->field_count; i++) {
        const db_field_t *f = &layout->fields[i];
        u8 *native = (u8 *)record + f->native;

        if (f->target < 0) {
            memcpy(native, bytes + f->file, f->size);
            continue;
        }
        for (s32 k = 0; k < f->count; k++) {
            ((void **)native)[k] = DB_FROM_ID(get_s32(bytes + f->file + 4 * k));
        }
    }
}

/* The row of a pointer in a table, or -1. The staff and player tables have
   1,325 rows past their count. */
static s32 row_in(const void *p, s32 table)
{
    const db_layout_t *t = &layouts[table];
    uintptr_t base = (uintptr_t)*t->base;
    uintptr_t at = (uintptr_t)p;
    s32 rows = *t->count + (table == DB_STAFF || table == DB_PLAYER ? DB_SPARE_ROWS : 0);

    if (base && at >= base && at < base + (uintptr_t)rows * t->native &&
        (at - base) % t->native == 0) {
        return (s32)((at - base) / t->native);
    }
    return -1;
}

/*
 * The row of a pointer in the table that a field points into, and -1 for
 * NULL. A pointer into another table, as the staff pass makes for a missing
 * first or second name, gives 0x80000000 | table << 24 | row. A pointer
 * field that still holds an id after the id pass gives 0x40000000 | id, so
 * that an id is never taken for the row of the same number.
 */
static s32 pointer_row(const void *p, s32 target)
{
    s32 row;

    if (!p) return -1;
    row = row_in(p, target);
    if (row >= 0) return row;
    for (s32 table = 0; table < DB_TABLE_IDS; table++) {
        if (table == DB_NAT_CLUB || table == DB_NATION_COMP || table == DB_NATION_COMP_HISTORY ||
            !layouts[table].fields) {
            continue;
        }
        row = row_in(p, table);
        if (row >= 0) return (s32)(0x80000000u | (u32)table << 24 | (u32)row);
    }
    return (s32)(0x40000000u | (u32)DB_ID(p));
}

static void record_to_bytes(const db_layout_t *layout, const void *record, u8 *bytes)
{
    for (s32 i = 0; i < layout->field_count; i++) {
        const db_field_t *f = &layout->fields[i];
        const u8 *native = (const u8 *)record + f->native;
        bool linked = g_links == LINKS_ALL || (g_links == LINKS_NATIONS && f->nation_link);

        if (f->target < 0) {
            memcpy(bytes + f->file, native, f->size);
            continue;
        }
        for (s32 k = 0; k < f->count; k++) {
            const void *p = ((void *const *)native)[k];
            put_s32(bytes + f->file + 4 * k, linked ? pointer_row(p, f->target) : DB_ID(p));
        }
    }
}

s32 database_table_info(s32 table, s32 *rows, s32 *record_size, db_link_t *links, s32 max)
{
    const db_layout_t *layout;
    s32 n = 0;

    if (table < 0 || table >= DB_TABLE_IDS || !layouts[table].fields) return -1;
    layout = &layouts[table];
    *rows = *layout->base ? *layout->count : 0;
    *record_size = layout->size;
    for (s32 i = 0; i < layout->field_count; i++) {
        const db_field_t *f = &layout->fields[i];
        if (f->target < 0) continue;
        if (n < max) {
            links[n].offset = f->file;
            links[n].count = f->count;
            links[n].target = f->target;
        }
        n++;
    }
    return n;
}

s32 database_check_layouts(void)
{
    s32 bad = 0;

    for (s32 table = 0; table < DB_TABLE_IDS; table++) {
        const db_layout_t *layout = &layouts[table];
        s32 end = 0;

        if (!layout->fields) continue;
        for (s32 i = 0; i < layout->field_count; i++) {
            const db_field_t *f = &layout->fields[i];
            size_t native = f->target < 0 ? f->size : f->count * sizeof(void *);

            if (f->file != end || f->native + native > layout->native) break;
            end += f->size;
        }
        if (end != layout->size) bad++;
    }
    return bad;
}

void database_record_bytes(s32 table, s32 row, u8 *out)
{
    const db_layout_t *layout = &layouts[table];

    record_to_bytes(layout, (const u8 *)*layout->base + (size_t)row * layout->native, out);
}

void database_nation_bytes(const nation_t *n, u8 out[DB_NATION_SIZE])
{
    record_to_bytes(&layouts[DB_NATION], n, out);
}

void database_continent_bytes(const continent_t *c, u8 out[DB_CONTINENT_SIZE])
{
    record_to_bytes(&layouts[DB_CONTINENT], c, out);
}

/* Files */

/* FUN_0054d580. Opens a file of the data folder, of any case. */
static FILE *open_data_file(const char *data_dir, const char *name)
{
    char *path = os_find_file_ci(data_dir, name);
    FILE *f;

    if (!path) return NULL;
    f = fopen(path, "rb");
    free(path);
    return f;
}

/*
 * Step 5 of FUN_00528540 (0x528734..0x528834): two ints, then 22 entries of
 * 0x43 bytes in one read. One more byte follows in a newer index. The
 * shipped file has none, and the byte then stays 0. When it is not 0 the
 * original reads a language block (0x528848..0x52896f), which the port does
 * not have, so the port fails.
 */
static bool read_index(const char *data_dir)
{
    u8 raw[DB_INDEX_ENTRIES * 0x43];
    u8 header[8];
    u8 extra = 0;
    FILE *f = open_data_file(data_dir, "index.dat");
    bool ok;

    if (!f) return false;
    ok = fread(header, 1, sizeof(header), f) == sizeof(header) &&
         fread(raw, 1, sizeof(raw), f) == sizeof(raw);
    if (ok && fread(&extra, 1, 1, f) == 1 && extra != 0) ok = false;
    fclose(f);
    if (!ok) return false;
    g_db_index_header[0] = get_s32(header);
    g_db_index_header[1] = get_s32(header + 4);
    for (s32 i = 0; i < DB_INDEX_ENTRIES; i++) {
        const u8 *e = raw + i * 0x43;
        db_index_entry_t *entry = &g_db_index[i];

        memcpy(entry->name, e, sizeof(entry->name));
        entry->name[sizeof(entry->name) - 1] = '\0';
        entry->table = get_s32(e + 0x33);
        entry->count = get_s32(e + 0x37);
        entry->offset = get_s32(e + 0x3b);
        entry->format = get_s32(e + 0x3f);
    }
    return true;
}

/* Readers */

/*
 * The formats that each reader of Db_files.cpp reads with a plain fread,
 * as a bit mask (bit 1: format 1, bit 2: format 2). The original converts
 * format 1 of the club, player, non-player and club comp tables
 * (FUN_0054dc90, FUN_0054f150, FUN_0054e9a0, FUN_0054ffd0), which the
 * shipped data does not have. The port fails there, and for any other
 * format, where the original shows an error box and ends.
 */
static u32 reader_formats(s32 table)
{
    switch (table) {
    case DB_CLUB: case DB_NAT_CLUB:                      /* FUN_0054db50 */
    case DB_NATION:                                      /* FUN_0054d740 */
    case DB_NON_PLAYER:                                  /* FUN_0054e880 */
    case DB_PLAYER:                                      /* FUN_0054f030 */
    case DB_STAFF_COMP:                                  /* FUN_00550380 */
    case DB_CLUB_COMP: case DB_NATION_COMP:              /* FUN_0054feb0 */
        return 1u << 2;
    case DB_CITY:                                        /* FUN_0054d8a0 */
        return 1u << 1 | 1u << 2;
    default:                                             /* format 1 only */
        return 1u << 1;
    }
}

/*
 * One reader: fread of `count` records into the table from the row `first`
 * of its layout. Returns false for a format it does not read or a short
 * read. The club reader FUN_0054db50 then gives a club with reputation 0
 * the reputation 1 (0x54dc35).
 */
static bool read_records(FILE *f, s32 table, s32 count, s32 format, void *rows)
{
    const db_layout_t *layout = &layouts[table];
    u8 *raw;
    size_t got;

    if (format < 1 || format > 2 || !(reader_formats(table) >> format & 1)) return false;
    if (count <= 0) return count == 0;
    raw = malloc((size_t)count * (size_t)layout->size);
    if (!raw) return false;
    got = fread(raw, (size_t)layout->size, (size_t)count, f);
    for (size_t i = 0; i < got; i++) {
        record_from_bytes(layout, (u8 *)rows + i * layout->native, raw + i * layout->size);
    }
    free(raw);
    if (got != (size_t)count) return false;
    if (table == DB_CLUB || table == DB_NAT_CLUB) {
        for (s32 i = 0; i < count; i++) {
            club_t *c = &((club_t *)rows)[i];
            if (c->reputation == 0) c->reputation = 1;
        }
    }
    return true;
}

/* The 157-byte staff reader FUN_0054e780: format 1 only. */
static bool read_staff_file(FILE *f, s32 count, s32 format)
{
    if (format != 1) return false;
    return fread(g_staff_file, DB_STAFF_FILE_SIZE, (size_t)count, f) == (size_t)count;
}

/* FUN_00528080 */
void database_free(void)
{
    free(g_continents);
    free(g_staff_histories);
    free(g_staff_comp_histories);
    free(g_comp_histories);
    free(g_preferences);
    free(g_staff_file);
    free(g_staff);
    free(g_players);
    free(g_non_players);
    free(g_officials);
    free(g_clubs);
    free(g_first_names);
    free(g_second_names);
    free(g_common_names);
    free(g_cities);
    free(g_staff_comps);
    free(g_club_comps);
    free(g_colours);
    free(g_nations);
    free(g_stadiums);
    g_clubs = g_nat_clubs = NULL;
    g_colours = NULL;
    g_continents = NULL;
    g_nations = NULL;
    g_stadiums = NULL;
    g_staff = NULL;
    g_officials = NULL;
    g_non_players = NULL;
    g_players = NULL;
    g_staff_comps = NULL;
    g_club_comps = g_nation_comps = NULL;
    g_first_names = g_second_names = g_common_names = NULL;
    g_staff_histories = NULL;
    g_staff_comp_histories = NULL;
    g_comp_histories = g_nation_comp_histories = NULL;
    g_cities = NULL;
    g_preferences = NULL;
    g_staff_file = NULL;
    g_club_count = g_nat_club_count = g_colour_count = g_continent_count = 0;
    g_nation_count = g_stadium_count = g_staff_count = g_official_count = 0;
    g_non_player_count = g_player_count = g_staff_comp_count = g_club_comp_count = 0;
    g_nation_comp_count = g_first_name_count = g_second_name_count = 0;
    g_common_name_count = g_staff_history_count = g_staff_comp_history_count = 0;
    g_comp_history_count = g_nation_comp_history_count = g_city_count = 0;
    g_preference_count = g_staff_file_count = 0;
    g_links = LINKS_NONE;
}

/* The nation and continent load of the league tests */

static const db_index_entry_t *find_entry(s32 table)
{
    for (s32 i = 0; i < DB_INDEX_ENTRIES; i++) {
        if (g_db_index[i].table == table) return &g_db_index[i];
    }
    return NULL;
}

static bool read_one_table(const char *data_dir, const db_index_entry_t *entry, s32 table)
{
    const db_layout_t *layout = &layouts[table];
    FILE *f;
    bool ok;

    if (!entry || entry->count < 0) return false;
    *layout->base = calloc((size_t)entry->count + 1, layout->native);
    if (!*layout->base) return false;
    *layout->count = entry->count;
    f = open_data_file(data_dir, entry->name);
    if (!f) return false;
    ok = fseek(f, entry->offset, SEEK_SET) == 0 &&
         read_records(f, table, entry->count, entry->format, *layout->base);
    fclose(f);
    return ok;
}

/*
 * The nation part of FUN_005315a0 (0x531f6a..0x5321ac) for the continent
 * and the rivals: a negative id gives NULL, any other id the record of that
 * row. The original does not check the upper end. The port gives NULL there.
 */
static void link_nations(void)
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
    g_links = LINKS_NATIONS;
}

bool database_load_nations(const char *data_dir)
{
    database_free();
    if (!read_index(data_dir) ||
        !read_one_table(data_dir, find_entry(DB_CONTINENT), DB_CONTINENT) ||
        !read_one_table(data_dir, find_entry(DB_NATION), DB_NATION)) {
        database_free();
        return false;
    }
    link_nations();
    return true;
}

/* The loader */

static void progress(s32 step)
{
    char text[0x100];

    text_format(text, sizeof(text), "Loading database");
    screen_stack_progress(&g_screen_stack, text, step, DB_PROGRESS_TOTAL, -1);
}

/* calloc of n rows of a table. The original does not clear the memory. */
static void *table_alloc(s32 rows, size_t size)
{
    return calloc(rows > 0 ? (size_t)rows : 1, size);
}

/*
 * The sizing loop of the shared branch (0x52990e..0x529cc0, jump table
 * 0x52d9d8): one progress step per entry. The counts add up, so that the
 * club and nat-club entries make one table, and so on. `tail` rows go to
 * the end of the staff, non-player and name tables: the manager rows.
 */
static bool size_tables(s32 tail, s32 *step)
{
    for (s32 i = 0; i < DB_INDEX_ENTRIES; i++) {
        const db_index_entry_t *e = &g_db_index[i];
        s32 n = e->count;

        progress((*step)++);
        switch (e->table) {
        case DB_CLUB: g_club_count += n; break;
        case DB_NAT_CLUB: g_nat_club_count += n; g_club_count += n; break;
        case DB_COLOUR: g_colour_count += n; break;
        case DB_CONTINENT: g_continent_count += n; break;
        case DB_NATION: g_nation_count = n; break;
        case DB_STADIUM: g_stadium_count = n; break;
        case DB_STAFF:                                         /* 0x529b31 */
            g_staff_count = n + tail + DB_RESERVE_STAFF + DB_SPARE_ROWS;
            if (e->format == 1) {
                g_staff_file_count = n;
                g_staff_file = malloc((size_t)(n > 0 ? n : 1) * DB_STAFF_FILE_SIZE);
                if (!g_staff_file) return false;
            }
            break;
        case DB_OFFICIAL: g_official_count += n; break;
        case DB_NON_PLAYER: g_non_player_count = n + tail + DB_RESERVE_STAFF; break;
        case DB_PLAYER: g_player_count = n + DB_SPARE_ROWS; break;
        case DB_STAFF_COMP: g_staff_comp_count += n; break;
        case DB_CLUB_COMP: g_club_comp_count += n + DB_RESERVE_COMPS; break;
        case DB_FIRST_NAME: g_first_name_count += n + tail; break;
        case DB_SECOND_NAME: g_second_name_count += n + tail; break;
        case DB_COMMON_NAME: g_common_name_count += n + tail; break;
        case DB_NATION_COMP: g_club_comp_count += n; g_nation_comp_count += n; break;
        case DB_STAFF_HISTORY: g_staff_history_count = n; break;
        case DB_STAFF_COMP_HISTORY: g_staff_comp_history_count += n; break;
        case DB_COMP_HISTORY: g_comp_history_count += n; break;
        case DB_NATION_COMP_HISTORY:
            g_comp_history_count += n;
            g_nation_comp_history_count += n;
            break;
        case DB_CITY: g_city_count += n; break;
        case DB_PREFERENCES:                                   /* never in index.dat */
            g_preference_count = n + tail;
            g_preferences = table_alloc(g_preference_count, sizeof(preferences_t));
            if (!g_preferences) return false;
            break;
        default: break;
        }
    }
    return true;
}

/* 0x529cc6..0x52a05a: the tables and the bases inside the shared arrays. */
static bool allocate_tables(void)
{
    g_staff_histories = table_alloc(g_staff_history_count, sizeof(staff_history_t));
    g_continents = table_alloc(g_continent_count, sizeof(continent_t));
    g_clubs = table_alloc(g_club_count, sizeof(club_t));
    g_staff = table_alloc(g_staff_count, sizeof(staff_t));
    g_players = table_alloc(g_player_count, sizeof(player_t));
    g_non_players = table_alloc(g_non_player_count, sizeof(non_player_t));
    g_staff_comp_histories = table_alloc(g_staff_comp_history_count, sizeof(staff_comp_history_t));
    g_comp_histories = table_alloc(g_comp_history_count, sizeof(comp_history_t));
    g_officials = table_alloc(g_official_count, sizeof(official_t));
    /* The original carves these from the block of the continents. */
    g_first_names = table_alloc(g_first_name_count, sizeof(db_name_t));
    g_second_names = table_alloc(g_second_name_count, sizeof(db_name_t));
    g_common_names = table_alloc(g_common_name_count, sizeof(db_name_t));
    g_cities = table_alloc(g_city_count, sizeof(city_t));
    g_staff_comps = table_alloc(g_staff_comp_count, sizeof(staff_comp_t));
    g_club_comps = table_alloc(g_club_comp_count, sizeof(club_comp_t));
    g_colours = table_alloc(g_colour_count, sizeof(colour_t));
    g_nations = table_alloc(g_nation_count, sizeof(nation_t));
    g_stadiums = table_alloc(g_stadium_count, sizeof(stadium_t));
    if (!g_staff_histories || !g_continents || !g_clubs || !g_staff || !g_players ||
        !g_non_players || !g_staff_comp_histories || !g_comp_histories || !g_officials ||
        !g_first_names || !g_second_names || !g_common_names || !g_cities || !g_staff_comps ||
        !g_club_comps || !g_colours || !g_nations || !g_stadiums) {
        return false;
    }
    g_nation_comps = g_club_comps + (g_club_comp_count - g_nation_comp_count - DB_RESERVE_COMPS);
    g_nation_comp_histories = g_comp_histories + (g_comp_history_count - g_nation_comp_history_count);
    g_nat_clubs = g_clubs + (g_club_count - g_nat_club_count);
    g_staff_count -= DB_SPARE_ROWS;                            /* 0x52a04b */
    g_player_count -= DB_SPARE_ROWS;
    return true;
}

/* 0x52a068: the last 127 rows of the club comp array. */
static void reserve_comps(void)
{
    for (s32 i = 1; i <= DB_RESERVE_COMPS; i++) {
        s32 row = g_club_comp_count - i;
        club_comp_t *r = &g_club_comps[row];

        r->id = row;
        r->name[0] = '\0';
        r->short_name[0] = '\0';
        r->b53 = 0;
        r->b57 = 0;
        r->b58 = 0;
        r->continent = DB_FROM_ID(-1);
        r->nation = DB_FROM_ID(-1);
        r->colours[0] = DB_FROM_ID(-1);
        r->colours[1] = DB_FROM_ID(-1);
        r->level = 0;
    }
}

/* The fields that a reserve row and a manager row share. */
static void blank_staff(staff_t *s, s32 row)
{
    s->id = row;
    date_default(&s->born);
    s->age = 1971;
    s->nation = DB_FROM_ID(-1);
    s->nation2 = DB_FROM_ID(-1);
    s->b22 = 0;
    s->b23 = 0;
    s->national_club = DB_FROM_ID(-1);
    s->national_job = 0;
    date_default(&s->d29);
    date_default(&s->d31);
    s->club = DB_FROM_ID(-1);
    s->club_job = 0;
    date_default(&s->contract_end);
    date_default(&s->d46);
    s->i4e = 0;
    s->i52 = 0;
    memset(s->unknown_56, 0, sizeof(s->unknown_56));
    s->b5e = 0;
    s->b60 = 0;
    s->player = DB_FROM_ID(-1);
    s->b6d = 0;
}

static void blank_non_player(non_player_t *np, s32 row)
{
    np->id = row;
    memset(np->unknown_04, 0, sizeof(np->unknown_04));
    memset(np->unknown_0e, 0, sizeof(np->unknown_0e));
    for (s32 k = 0; k < 8; k++) np->staff[k] = DB_FROM_ID(-1);
    np->unknown_43 = 0;
}

static void blank_name(db_name_t *n, s32 row)
{
    n->name[0] = '\0';
    n->id = row;
    n->nation_id = -1;
    n->count = 1;
}

/*
 * 0x52a1c2: 23,000 reserve staff rows before the manager rows, each with
 * its own non-player row. Then 0x52a9d3: the `tail` manager rows at the
 * very end, with their own name rows and non-player rows.
 */
static void reserve_staff(s32 tail)
{
    for (s32 i = 1; i <= DB_RESERVE_STAFF; i++) {
        staff_t *s = &g_staff[g_staff_count - i - tail];
        s32 np = g_non_player_count - i - tail;

        blank_staff(s, g_staff_count - i - tail);
        s->first_name = DB_FROM_ID(-1);
        s->second_name = DB_FROM_ID(-1);
        s->common_name = DB_FROM_ID(0);
        s->b5f = 6;
        s->prefs = DB_FROM_ID(-1);
        s->non_player = DB_FROM_ID(np);
        blank_non_player(&g_non_players[np], np);
    }
}

static void manager_rows(s32 tail)
{
    for (s32 i = 1; i <= tail; i++) {
        staff_t *s = &g_staff[g_staff_count - i];
        s32 np = g_non_player_count - i;

        blank_staff(s, g_staff_count - i);
        s->first_name = DB_FROM_ID(g_first_name_count - i);
        s->second_name = DB_FROM_ID(g_second_name_count - i);
        s->common_name = DB_FROM_ID(g_common_name_count - i);
        s->b5f = 0;
        s->prefs = DB_FROM_ID(g_preference_count - i);   /* the count is still 0 */
        s->non_player = DB_FROM_ID(np);
        blank_name(&g_first_names[g_first_name_count - i], g_first_name_count - i);
        blank_name(&g_second_names[g_second_name_count - i], g_second_name_count - i);
        blank_name(&g_common_names[g_common_name_count - i], g_common_name_count - i);
        blank_non_player(&g_non_players[np], np);
        if (g_preferences) {                                   /* 0x52b249 */
            preferences_t *p = &g_preferences[g_preference_count - i];
            p->id = g_preference_count - i;
            for (s32 k = 0; k < 6; k++) {
                p->clubs[k] = DB_FROM_ID(-1);
                p->staff[k] = DB_FROM_ID(-1);
            }
        }
    }
}

/* The rows of a table where a read starts. */
static void *table_rows(s32 table)
{
    return *layouts[table].base;
}

/*
 * The read loop (0x52b37c, jump table 0x52da34): one progress step per
 * entry, then open the file, seek and call the reader. A table that the
 * sizing gave no memory is skipped. A missing file fails the load, but for
 * the staff history and the staff comp history, whose tables are then
 * freed and counted 0.
 */
static bool read_tables(const char *data_dir, s32 *step)
{
    for (s32 i = 0; i < DB_INDEX_ENTRIES; i++) {
        const db_index_entry_t *e = &g_db_index[i];
        s32 table = e->table;
        FILE *f;
        bool ok;

        progress((*step)++);
        if (table < 0 || table >= DB_PREFERENCES || table == DB_STAFF_B) continue;
        if (table == DB_STAFF ? !g_staff_file && !g_staff : !table_rows(table)) continue;
        f = open_data_file(data_dir, e->name);
        if (!f) {
            if (table == DB_STAFF_HISTORY) {                   /* 0x52c58b */
                free(g_staff_histories);
                g_staff_histories = NULL;
                g_staff_history_count = 0;
                continue;
            }
            if (table == DB_STAFF_COMP_HISTORY) {              /* 0x52bce4 */
                free(g_staff_comp_histories);
                g_staff_comp_histories = NULL;
                g_staff_comp_history_count = 0;
                continue;
            }
            return false;
        }
        fseek(f, e->offset, SEEK_SET);
        if (table == DB_STAFF) {
            /* The 110-byte reader FUN_0054e630 for a staff entry of another
               format is not ported. */
            ok = g_staff_file && read_staff_file(f, e->count, e->format);
        } else {
            ok = read_records(f, table, e->count, e->format, table_rows(table));
        }
        /* 0x52bbd4: a second read of nation comps into the reserve rows when
           the first int of index.dat is above 0. */
        if (ok && table == DB_NATION_COMP && g_db_index_header[0] > 0) {
            ok = read_records(f, DB_CLUB_COMP, g_db_index_header[0], e->format,
                              &g_club_comps[g_club_comp_count - DB_RESERVE_COMPS]);
        }
        fclose(f);
        if (!ok) return false;
    }
    return true;
}

/*
 * 0x52caa1: the 157-byte staff records become 110-byte records, and their
 * twelve ints at 0x61 become the preferences table, one row per staff row,
 * with `tail` blank rows at the end.
 */
static bool convert_staff(void)
{
    if (!g_staff_file) return true;
    g_preference_count = g_staff_file_count + g_db_index_header[1];
    free(g_preferences);
    g_preferences = table_alloc(g_preference_count, sizeof(preferences_t));
    if (!g_preferences) return false;
    for (s32 i = 0; i < g_staff_file_count; i++) {
        const u8 *src = g_staff_file + (size_t)i * DB_STAFF_FILE_SIZE;
        staff_t *dst = &g_staff[i];
        preferences_t *p = &g_preferences[i];

        dst->id = get_s32(src);
        dst->first_name = DB_FROM_ID(get_s32(src + 0x04));
        dst->second_name = DB_FROM_ID(get_s32(src + 0x08));
        dst->common_name = DB_FROM_ID(get_s32(src + 0x0c));
        memcpy(&dst->born, src + 0x10, sizeof(dst->born));
        memcpy(&dst->age, src + 0x18, sizeof(dst->age));
        dst->nation = DB_FROM_ID(get_s32(src + 0x1a));
        dst->nation2 = DB_FROM_ID(get_s32(src + 0x1e));
        dst->b22 = src[0x22];
        dst->b23 = src[0x23];
        dst->club = DB_FROM_ID(get_s32(src + 0x39));
        dst->club_job = (s8)src[0x3d];
        memcpy(&dst->contract_end, src + 0x3e, sizeof(dst->contract_end));
        memcpy(&dst->d46, src + 0x46, sizeof(dst->d46));
        dst->national_club = DB_FROM_ID(get_s32(src + 0x24));
        dst->national_job = (s8)src[0x28];
        memcpy(&dst->d29, src + 0x29, sizeof(dst->d29));
        memcpy(&dst->d31, src + 0x31, sizeof(dst->d31));
        dst->i4e = get_s32(src + 0x4e);
        dst->i52 = get_s32(src + 0x52);
        memcpy(dst->unknown_56, src + 0x56, sizeof(dst->unknown_56));
        dst->b5e = src[0x5e];
        dst->b5f = src[0x5f];
        dst->player = DB_FROM_ID(get_s32(src + 0x91));
        dst->prefs = DB_FROM_ID(i);
        dst->non_player = DB_FROM_ID(get_s32(src + 0x99));
        dst->b60 = src[0x60];
        p->id = i;
        for (s32 k = 0; k < 6; k++) {
            p->clubs[k] = DB_FROM_ID(get_s32(src + 0x61 + 4 * k));
            p->staff[k] = DB_FROM_ID(get_s32(src + 0x79 + 4 * k));
        }
    }
    for (s32 i = 1; i <= g_db_index_header[1]; i++) {           /* 0x52d037 */
        preferences_t *p = &g_preferences[g_preference_count - i];

        p->id = g_preference_count - i;
        for (s32 k = 0; k < 6; k++) {
            p->clubs[k] = DB_FROM_ID(-1);
            p->staff[k] = DB_FROM_ID(-1);
        }
    }
    free(g_staff_file);
    g_staff_file = NULL;
    g_staff_file_count = 0;
    return true;
}

/*
 * FUN_00528540(dir, 1, 0) up to the call of the id pass (0x52d1aa). Left
 * out: the copy of the .lng files to CM3_TEMP and the language check
 * FUN_00535930 (0x52861f..0x5286fc), the language block of a newer index,
 * the non-shared sizing of a saved game (0x5289c7) and the read from a
 * stream.
 */
bool database_read(const char *data_dir)
{
    s32 step = 0, tail;

    database_free();                                           /* FUN_00528080 */
    progress(step++);                                          /* 0x5285d4 */
    if (!read_index(data_dir)) return false;
    tail = DB_MANAGER_ROWS - g_db_index_header[1];             /* 0x52898d */
    progress(step++);                                          /* 0x5289a9 */
    g_db_shared = 1;
    if (!size_tables(tail, &step) || !allocate_tables()) {
        database_free();
        return false;
    }
    reserve_comps();
    reserve_staff(tail);
    manager_rows(tail);
    if (!read_tables(data_dir, &step)) {
        database_free();
        return false;
    }
    progress(step);                                            /* 0x52c671 */
    g_db_index_header[0] = DB_RESERVE_COMPS;                   /* 0x52c687 */
    g_db_index_header[1] += tail;
    if (!convert_staff()) {
        database_free();
        return false;
    }
    return true;
}

/*
 * FUN_00524d50. The id pass, the staff pass and the national clubs are in
 * database_links.c, the key ids in key_index.c. The delete of duff.out
 * (0x52d205) is left out.
 */
bool database_load(const char *data_dir)
{
    if (!database_read(data_dir)) return false;
    database_resolve();                                        /* FUN_005315a0 */
    database_staff_pass();                                     /* 0x52d20a */
    database_national_clubs();                                 /* 0x52d82c */
    key_index_set();                                           /* FUN_00601a40 */
    return true;
}

/* Called by the id pass of database_links.c. */
void database_links_done(void)
{
    g_links = LINKS_ALL;
}
