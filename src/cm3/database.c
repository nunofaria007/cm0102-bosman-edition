#include "database.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

extern char *os_find_file_ci(const char *dir, const char *name);

/*
 * The nation and continent part of the loader of Database.cpp and the
 * readers of Db_files.cpp. See database.h.
 */

s32 g_db_index_header[2];
db_index_entry_t g_db_index[DB_INDEX_ENTRIES];

continent_t *g_continents;
s32 g_continent_count;
nation_t *g_nations;
s32 g_nation_count;

static s32 get_s32(const u8 *p)
{
    return (s32)((u32)p[0] | (u32)p[1] << 8 | (u32)p[2] << 16 | (u32)p[3] << 24);
}

static void put_s32(u8 *p, s32 value)
{
    for (s32 i = 0; i < 4; i++) p[i] = (u8)((u32)value >> (8 * i));
}

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

static const db_index_entry_t *find_entry(s32 table)
{
    for (s32 i = 0; i < DB_INDEX_ENTRIES; i++) {
        if (g_db_index[i].table == table) return &g_db_index[i];
    }
    return NULL;
}

/*
 * The read step of the loop at 0x52b3ab for one table: open the file of the
 * entry, seek to its offset and read count records raw. Returns the raw
 * records, or NULL.
 */
static u8 *read_table(const char *data_dir, const db_index_entry_t *entry, s32 size)
{
    FILE *f;
    u8 *raw;
    size_t count;

    if (!entry || entry->count < 0) return NULL;
    f = open_data_file(data_dir, entry->name);
    if (!f) return NULL;
    raw = malloc((size_t)entry->count * (size_t)size + 1);
    if (raw && fseek(f, entry->offset, SEEK_SET) != 0) {
        free(raw);
        raw = NULL;
    }
    if (raw) {
        count = fread(raw, (size_t)size, (size_t)entry->count, f);
        if (count != (size_t)entry->count) {
            free(raw);
            raw = NULL;
        }
    }
    fclose(f);
    return raw;
}

static void continent_from_bytes(continent_t *c, const u8 *p)
{
    c->id = get_s32(p);
    memcpy(c->name, p + 0x04, sizeof(c->name));
    c->unknown_1e = p[0x1e];
    memcpy(c->code, p + 0x1f, sizeof(c->code));
    memcpy(c->adjective, p + 0x23, sizeof(c->adjective));
    memcpy(c->long_name, p + 0x3d, sizeof(c->long_name));
    c->unknown_a2 = p[0xa2];
    memcpy(c->short_code, p + 0xa3, sizeof(c->short_code));
    c->unknown_bd = p[0xbd];
    memcpy(&c->unknown_be, p + 0xbe, sizeof(c->unknown_be));
}

void database_continent_bytes(const continent_t *c, u8 out[DB_CONTINENT_SIZE])
{
    put_s32(out, c->id);
    memcpy(out + 0x04, c->name, sizeof(c->name));
    out[0x1e] = c->unknown_1e;
    memcpy(out + 0x1f, c->code, sizeof(c->code));
    memcpy(out + 0x23, c->adjective, sizeof(c->adjective));
    memcpy(out + 0x3d, c->long_name, sizeof(c->long_name));
    out[0xa2] = c->unknown_a2;
    memcpy(out + 0xa3, c->short_code, sizeof(c->short_code));
    out[0xbd] = c->unknown_bd;
    memcpy(out + 0xbe, &c->unknown_be, sizeof(c->unknown_be));
}

/*
 * The nation part of FUN_005315a0 (0x531f6a..0x5321ac): a negative id gives
 * NULL, any other id the record of that row. The original does not check
 * the upper end. The port gives NULL there too.
 */
static continent_t *link_continent(s32 id)
{
    return id < 0 || id >= g_continent_count ? NULL : &g_continents[id];
}

static nation_t *link_nation(s32 id)
{
    return id < 0 || id >= g_nation_count ? NULL : &g_nations[id];
}

static void nation_from_bytes(nation_t *n, const u8 *p)
{
    n->id = get_s32(p);
    memcpy(n->name, p + 0x04, sizeof(n->name));
    n->unknown_037 = p[0x37];
    memcpy(n->second_name, p + 0x38, sizeof(n->second_name));
    n->unknown_052 = p[0x52];
    memcpy(n->code, p + 0x53, sizeof(n->code));
    memcpy(n->nationality, p + 0x57, sizeof(n->nationality));
    n->continent = link_continent(get_s32(p + 0x71));
    memcpy(n->unknown_075, p + 0x75, sizeof(n->unknown_075));
    n->city_id = get_s32(p + 0x7a);
    memcpy(n->unknown_07e, p + 0x7e, sizeof(n->unknown_07e));
    n->stadium_id = get_s32(p + 0x80);
    memcpy(n->unknown_084, p + 0x84, sizeof(n->unknown_084));
    for (s32 i = 0; i < 6; i++) n->colour_ids[i] = get_s32(p + 0x90 + 4 * i);
    memcpy(n->unknown_0a8, p + 0xa8, sizeof(n->unknown_0a8));
    for (s32 i = 0; i < 3; i++) n->rivals[i] = link_nation(get_s32(p + 0x110 + 4 * i));
    n->league_flags = p[0x11c];
    memcpy(n->unknown_11d, p + 0x11d, sizeof(n->unknown_11d));
}

void database_nation_bytes(const nation_t *n, u8 out[DB_NATION_SIZE])
{
    put_s32(out, n->id);
    memcpy(out + 0x04, n->name, sizeof(n->name));
    out[0x37] = n->unknown_037;
    memcpy(out + 0x38, n->second_name, sizeof(n->second_name));
    out[0x52] = n->unknown_052;
    memcpy(out + 0x53, n->code, sizeof(n->code));
    memcpy(out + 0x57, n->nationality, sizeof(n->nationality));
    put_s32(out + 0x71, n->continent ? (s32)(n->continent - g_continents) : -1);
    memcpy(out + 0x75, n->unknown_075, sizeof(n->unknown_075));
    put_s32(out + 0x7a, n->city_id);
    memcpy(out + 0x7e, n->unknown_07e, sizeof(n->unknown_07e));
    put_s32(out + 0x80, n->stadium_id);
    memcpy(out + 0x84, n->unknown_084, sizeof(n->unknown_084));
    for (s32 i = 0; i < 6; i++) put_s32(out + 0x90 + 4 * i, n->colour_ids[i]);
    memcpy(out + 0xa8, n->unknown_0a8, sizeof(n->unknown_0a8));
    for (s32 i = 0; i < 3; i++) {
        put_s32(out + 0x110 + 4 * i, n->rivals[i] ? (s32)(n->rivals[i] - g_nations) : -1);
    }
    out[0x11c] = n->league_flags;
    memcpy(out + 0x11d, n->unknown_11d, sizeof(n->unknown_11d));
}

void database_free(void)
{
    free(g_continents);
    free(g_nations);
    g_continents = NULL;
    g_nations = NULL;
    g_continent_count = 0;
    g_nation_count = 0;
}

bool database_load_nations(const char *data_dir)
{
    const db_index_entry_t *continents, *nations;
    u8 *raw_continents = NULL, *raw_nations = NULL;
    bool ok = false;

    database_free();
    if (!read_index(data_dir)) return false;
    continents = find_entry(DB_TABLE_CONTINENT);
    nations = find_entry(DB_TABLE_NATION);
    /* FUN_0054d5e0 takes format 1 or 2, FUN_0054d740 only 2. */
    if (!continents || (continents->format != 1 && continents->format != 2)) return false;
    if (!nations || nations->format != 2) return false;
    raw_continents = read_table(data_dir, continents, DB_CONTINENT_SIZE);
    raw_nations = read_table(data_dir, nations, DB_NATION_SIZE);
    g_continents = calloc((size_t)continents->count + 1, sizeof(continent_t));
    g_nations = calloc((size_t)nations->count + 1, sizeof(nation_t));
    if (raw_continents && raw_nations && g_continents && g_nations) {
        g_continent_count = continents->count;
        g_nation_count = nations->count;
        for (s32 i = 0; i < g_continent_count; i++) {
            continent_from_bytes(&g_continents[i], raw_continents + i * DB_CONTINENT_SIZE);
        }
        for (s32 i = 0; i < g_nation_count; i++) {
            nation_from_bytes(&g_nations[i], raw_nations + i * DB_NATION_SIZE);
        }
        ok = true;
    }
    free(raw_continents);
    free(raw_nations);
    if (!ok) database_free();
    return ok;
}
