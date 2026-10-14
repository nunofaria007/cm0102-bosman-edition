#ifndef DATABASE_H
#define DATABASE_H

#include "cm_types.h"

/*
 * The database of the original, from Database.cpp (the loader FUN_00528540)
 * and Db_files.cpp (one reader for each table). This part loads index.dat,
 * the continent table and the nation table, which "Select League(s)" needs.
 * The other 19 tables wait for the spec that creates the game.
 *
 * API for the screens:
 *   database_load_nations(dir)  reads index.dat, continent.dat and
 *                               nation.dat of the data folder `dir` and links
 *                               each nation to its continent and rivals.
 *   g_nations, g_nation_count   the nation table, in file order. Row i has
 *                               id i in the shipped data.
 *   g_continents, ...           the continent table.
 *   database_free()             frees the tables.
 * Then key_index_match() (key_index.h) finds the key nations by name, and
 * league_table_build() (key_nation.h) builds the league table.
 *
 * Records are native structs. The comments give the offset of each field in
 * the 1-byte packed record of the original. The original reads the record
 * raw and then FUN_005315a0 replaces each id field with a pointer into the
 * target table, or NULL for a negative id. The port does the same with real
 * pointers for the tables that it loads (continent, nation). For the tables
 * that it does not load yet (city, stadium, colour) it keeps the id.
 */

#define DB_INDEX_ENTRIES 22       /* entries of index.dat, a fixed count */
#define DB_TABLE_CONTINENT 3      /* table ids at index entry +0x33 */
#define DB_TABLE_NATION 4
#define DB_CONTINENT_SIZE 0xc6    /* record sizes in the files */
#define DB_NATION_SIZE 0x122

/* One entry of index.dat, 0x43 bytes. The entry table is 0xae1a80. */
typedef struct {
    char name[51];   /* +0x00 file name. Bytes after the NUL are junk */
    s32 table;       /* +0x33 table id, which the loader switches on */
    s32 count;       /* +0x37 record count */
    s32 offset;      /* +0x3b byte offset of the table in the file */
    s32 format;      /* +0x3f 1 or 2 */
} db_index_entry_t;

/* Continent, 198 bytes (DAT_00ae23a4, count DAT_00ae234c). */
typedef struct continent {
    s32 id;               /* +0x00 equals the row */
    char name[26];        /* +0x04 "Europe" */
    u8 unknown_1e;        /* +0x1e 0xff in the data */
    char code[4];         /* +0x1f "EUR" */
    char adjective[26];   /* +0x23 "European" */
    char long_name[101];  /* +0x3d "Union of European Football Associations" */
    u8 unknown_a2;        /* +0xa2 0xff */
    char short_code[26];  /* +0xa3 "UEFA" */
    u8 unknown_bd;        /* +0xbd 0xff */
    double unknown_be;    /* +0xbe 0.9 to 1.0 in the data */
} continent_t;

/*
 * Nation, 290 bytes (DAT_00ae23a8, count DAT_00ae2350). The sizes of the
 * text fields after the name come from the data, not from the code.
 */
typedef struct nation {
    s32 id;                    /* +0x000 equals the row */
    char name[51];             /* +0x004 "England" */
    u8 unknown_037;            /* +0x037 0xff */
    char second_name[26];      /* +0x038 sort key of "Select Start Season" */
    u8 unknown_052;            /* +0x052 0xff */
    char code[4];              /* +0x053 "ENG" */
    char nationality[26];      /* +0x057 "English" */
    continent_t *continent;    /* +0x071 id, then pointer */
    u8 unknown_075[5];         /* +0x075 */
    s32 city_id;               /* +0x07a id of city.dat, not resolved yet */
    u8 unknown_07e[2];         /* +0x07e */
    s32 stadium_id;            /* +0x080 id of stadium.dat, not resolved yet */
    u8 unknown_084[12];        /* +0x084 */
    s32 colour_ids[6];         /* +0x090 ids of colour.dat, not resolved yet */
    double unknown_0a8[13];    /* +0x0a8 */
    struct nation *rivals[3];  /* +0x110 id, then pointer. Meaning inferred */
    u8 league_flags;           /* +0x11c run time. Bit 0 background, bit 1
                                  selected, bit 2 extra division */
    u8 unknown_11d[5];         /* +0x11d */
} nation_t;

/* The two ints at the start of index.dat (DAT_00ae2344, DAT_00ae2348). */
extern s32 g_db_index_header[2];
extern db_index_entry_t g_db_index[DB_INDEX_ENTRIES];

extern continent_t *g_continents;   /* DAT_00ae23a4 */
extern s32 g_continent_count;       /* DAT_00ae234c */
extern nation_t *g_nations;         /* DAT_00ae23a8 */
extern s32 g_nation_count;          /* DAT_00ae2350 */

/*
 * Steps 1, 5 and 7 of FUN_00528540 for these two tables: frees the old
 * tables, reads index.dat, then each table at the offset that its entry
 * gives, then links the nations (the nation part of FUN_005315a0). Returns
 * false when a file is missing or short, or a format is wrong. The original
 * shows an error box and ends for a wrong format. The progress box, the
 * copy of the .lng files and the other tables are not here.
 */
bool database_load_nations(const char *data_dir);

/* The part of FUN_00528080 for these tables. */
void database_free(void);

/*
 * The record as the file holds it, with the id behind each pointer: the
 * row of the target, or -1 for NULL. For the differential test.
 */
void database_nation_bytes(const nation_t *n, u8 out[DB_NATION_SIZE]);
void database_continent_bytes(const continent_t *c, u8 out[DB_CONTINENT_SIZE]);

#endif
