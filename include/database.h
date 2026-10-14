#ifndef DATABASE_H
#define DATABASE_H

#include "cm_types.h"
#include <stddef.h>

/*
 * The database of the original, from Database.cpp (the loader FUN_00528540)
 * and Db_files.cpp (one reader for each table).
 *
 * API for the screens:
 *   database_load(dir)          the load of "Start New Game" (FUN_00524d50):
 *                               all 22 entries of index.dat, the reserve
 *                               rows, the staff conversion, the preferences
 *                               table, the id pass, the staff pass, the
 *                               national clubs and the key ids.
 *   g_clubs, g_club_count, ...  the tables, one global pair for each global
 *                               pair of the original.
 *   database_free()             frees the tables.
 *   database_load_nations(dir)  only index.dat, the continents and the
 *                               nations, for the league tests.
 *
 * Records are native structs. The comment of each field gives its offset in
 * the 1-byte packed record of the original. The original reads each record
 * raw, so an id field holds the row of its target. FUN_005315a0 then puts a
 * pointer to the row in its place, or NULL. The port does the same with real
 * pointers: until the id pass a pointer field holds its id, cast with
 * DB_FROM_ID. Some fields keep the id after the pass, as in the original
 * (see database_resolve). Unknown fields keep the bytes of the file.
 */

#define DB_INDEX_ENTRIES 22       /* entries of index.dat, a fixed count */

/* Table ids at index entry +0x33, which the loader switches on. */
enum db_table {
    DB_CLUB = 0,
    DB_NAT_CLUB = 1,
    DB_COLOUR = 2,
    DB_CONTINENT = 3,
    DB_NATION = 4,
    DB_STADIUM = 5,
    DB_STAFF = 6,                 /* 157-byte records, made into 110 bytes */
    DB_OFFICIAL = 7,
    DB_STAFF_B = 8,               /* no reader, no case */
    DB_NON_PLAYER = 9,
    DB_PLAYER = 10,
    DB_STAFF_COMP = 11,
    DB_CLUB_COMP = 12,
    DB_FIRST_NAME = 13,
    DB_SECOND_NAME = 14,
    DB_COMMON_NAME = 15,
    DB_NATION_COMP = 16,
    DB_STAFF_HISTORY = 17,
    DB_STAFF_COMP_HISTORY = 18,
    DB_COMP_HISTORY = 19,
    DB_NATION_COMP_HISTORY = 20,
    DB_CITY = 21,
    DB_PREFERENCES = 22,          /* built by the loader, never in index.dat */
    DB_TABLE_IDS
};

/* Record sizes in the files and in the memory of the original. */
#define DB_CLUB_SIZE 0x245
#define DB_COLOUR_SIZE 0x3a
#define DB_CONTINENT_SIZE 0xc6
#define DB_NATION_SIZE 0x122
#define DB_STADIUM_SIZE 0x4e
#define DB_STAFF_FILE_SIZE 0x9d
#define DB_STAFF_SIZE 0x6e
#define DB_OFFICIAL_SIZE 0x2b
#define DB_NON_PLAYER_SIZE 0x44
#define DB_PLAYER_SIZE 0x46
#define DB_STAFF_COMP_SIZE 0x65
#define DB_CLUB_COMP_SIZE 0x6b
#define DB_NAME_SIZE 0x3c
#define DB_STAFF_HISTORY_SIZE 0x11
#define DB_STAFF_COMP_HISTORY_SIZE 0x3a
#define DB_COMP_HISTORY_SIZE 0x1a
#define DB_CITY_SIZE 0x38
#define DB_PREFERENCES_SIZE 0x34

/* The id of a pointer field before the id pass, and back. */
#define DB_FROM_ID(id) ((void *)(intptr_t)(s32)(id))
#define DB_ID(p) ((s32)(intptr_t)(p))

/* One entry of index.dat, 0x43 bytes. The entry table is 0xae1a80. */
typedef struct {
    char name[51];   /* +0x00 file name. Bytes after the NUL are junk */
    s32 table;       /* +0x33 table id, enum db_table */
    s32 count;       /* +0x37 record count */
    s32 offset;      /* +0x3b byte offset of the table in the file */
    s32 format;      /* +0x3f 1 or 2 */
} db_index_entry_t;

/*
 * A date, 8 bytes: the day of the year (0 for 1 Jan), the year, 1 in a
 * leap year, and 0. The dates and their functions belong to the dates
 * slice of the game setup spec. The load needs only FUN_0054a110 and two
 * dates of FUN_00549f70, which database.c has.
 */
typedef struct {
    s16 day;
    u16 year;
    u16 leap;
    u16 zero;
} db_date_t;

typedef struct club club_t;
typedef struct colour colour_t;
typedef struct continent continent_t;
typedef struct nation nation_t;
typedef struct stadium stadium_t;
typedef struct staff staff_t;
typedef struct official official_t;
typedef struct non_player non_player_t;
typedef struct player player_t;
typedef struct staff_comp staff_comp_t;
typedef struct club_comp club_comp_t;
typedef struct db_name db_name_t;
typedef struct staff_history staff_history_t;
typedef struct staff_comp_history staff_comp_history_t;
typedef struct comp_history comp_history_t;
typedef struct city city_t;
typedef struct preferences preferences_t;

/* Continent, 198 bytes (DAT_00ae23a4, count DAT_00ae234c). */
struct continent {
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
};

/*
 * Nation, 290 bytes (DAT_00ae23a8, count DAT_00ae2350). The sizes of the
 * text fields after the name come from the data, not from the code.
 */
struct nation {
    s32 id;                    /* +0x000 equals the row */
    char name[51];             /* +0x004 "England" */
    u8 unknown_037;            /* +0x037 0xff */
    char second_name[26];      /* +0x038 sort key of "Select Start Season" */
    u8 unknown_052;            /* +0x052 0xff */
    char code[4];              /* +0x053 "ENG" */
    char nationality[26];      /* +0x057 "English" */
    continent_t *continent;    /* +0x071 */
    u8 unknown_075[5];         /* +0x075 */
    city_t *city;              /* +0x07a */
    u8 unknown_07e[2];         /* +0x07e */
    stadium_t *stadium;        /* +0x080 */
    u8 unknown_084[10];        /* +0x084 */
    s16 unknown_08e;           /* +0x08e */
    colour_t *colours[6];      /* +0x090 */
    double unknown_0a8[13];    /* +0x0a8 */
    nation_t *rivals[3];       /* +0x110 meaning inferred */
    u8 league_flags;           /* +0x11c run time. Bit 0 background, bit 1
                                  selected, bit 2 extra division */
    u8 unknown_11d[5];         /* +0x11d */
};

/* Club and national club, 581 bytes (DAT_00ae23b4, count DAT_00ae235c). */
struct club {
    s32 id;                    /* +0x000 */
    char name[51];             /* +0x004 */
    u8 unknown_037;            /* +0x037 */
    char short_name[26];       /* +0x038 */
    u8 unknown_052;            /* +0x052 */
    nation_t *nation;          /* +0x053 NULL for an extinct club */
    club_comp_t *division;     /* +0x057 */
    club_comp_t *last_division; /* +0x05b keeps a too high id (id pass) */
    u8 unknown_05f;            /* +0x05f */
    club_comp_t *comp_060;     /* +0x060 */
    u8 unknown_064[5];         /* +0x064 */
    stadium_t *stadium;        /* +0x069 */
    u8 unknown_06d;            /* +0x06d */
    stadium_t *stadium_06e;    /* +0x06e inference: second stadium */
    u8 unknown_072;            /* +0x072 */
    s32 unknown_073;           /* +0x073 inference: money values */
    s32 unknown_077;           /* +0x077 */
    s32 unknown_07b;           /* +0x07b */
    u8 unknown_07f;            /* +0x07f */
    s16 reputation;            /* +0x080 0 becomes 1 in the reader */
    u8 unknown_082;            /* +0x082 */
    colour_t *colours[6];      /* +0x083 */
    staff_t *staff_09b[6];     /* +0x09b meaning unknown */
    club_t *clubs_0b3[3];      /* +0x0b3 inference: rivals */
    staff_t *chairman;         /* +0x0bf */
    staff_t *directors[3];     /* +0x0c3 */
    staff_t *manager;          /* +0x0cf */
    staff_t *assistant;        /* +0x0d3 */
    staff_t *squad[50];        /* +0x0d7 */
    staff_t *coaches[5];       /* +0x19f */
    staff_t *scouts[7];        /* +0x1b3 */
    staff_t *physios[3];       /* +0x1cf */
    s32 unknown_1db;           /* +0x1db */
    u8 unknown_1df;            /* +0x1df */
    staff_t *team[20];         /* +0x1e0 "team member selected" */
    u8 unknown_230[20];        /* +0x230 */
    u8 in_league;              /* +0x244 set by game creation, 0 in the files */
};

/* Colour, 58 bytes (DAT_00ae23ec, count DAT_00ae2394). */
struct colour {
    s32 id;                    /* +0x00 */
    char name[51];             /* +0x04 */
    u8 unknown_37[3];          /* +0x37 */
};

/* Stadium, 78 bytes (DAT_00ae23b0, count DAT_00ae2358). */
struct stadium {
    s32 id;                    /* +0x00 */
    char name[51];             /* +0x04 */
    u8 unknown_37;             /* +0x37 */
    city_t *city;              /* +0x38 */
    u8 unknown_3c[12];         /* +0x3c */
    stadium_t *stadium_48;     /* +0x48 keeps a too high id (id pass) */
    u8 unknown_4c[2];          /* +0x4c */
};

/*
 * Staff, 110 bytes in memory (DAT_00ae23bc, count DAT_00ae2364). The file
 * has 157 bytes: 0x00..0x60 as here, twelve ints at 0x61 that go to the
 * preferences row, the player id at 0x91 and the non-player id at 0x99.
 */
struct staff {
    s32 id;                    /* +0x00 */
    db_name_t *first_name;     /* +0x04 */
    db_name_t *second_name;    /* +0x08 */
    db_name_t *common_name;    /* +0x0c */
    db_date_t born;            /* +0x10 */
    s16 age;                   /* +0x18 inference */
    nation_t *nation;          /* +0x1a */
    nation_t *nation2;         /* +0x1e inference: second nationality */
    u8 b22;                    /* +0x22 */
    u8 b23;                    /* +0x23 cleared when +0x22 is 0 */
    club_t *national_club;     /* +0x24 */
    s8 national_job;           /* +0x28 codes as club_job */
    db_date_t d29;             /* +0x29 */
    db_date_t d31;             /* +0x31 */
    club_t *club;              /* +0x39 */
    s8 club_job;               /* +0x3d 1..10 non-player, 11 player, 12, 13
                                  and 15 both, 14 none */
    db_date_t contract_end;    /* +0x3e inference */
    db_date_t d46;             /* +0x46 */
    s32 i4e;                   /* +0x4e */
    s32 i52;                   /* +0x52 */
    u8 unknown_56[8];          /* +0x56 */
    u8 b5e;                    /* +0x5e */
    u8 b5f;                    /* +0x5f reserve rows 6. Manager rows: 0
                                  free, 5 being made, 1 human manager */
    u8 b60;                    /* +0x60 */
    player_t *player;          /* +0x61 */
    preferences_t *prefs;      /* +0x65 */
    non_player_t *non_player;  /* +0x69 */
    u8 b6d;                    /* +0x6d */
};

/* Official, 43 bytes (DAT_00ae23e8, count DAT_00ae2390). */
struct official {
    s32 id;                    /* +0x00 */
    db_name_t *first_name;     /* +0x04 */
    db_name_t *second_name;    /* +0x08 */
    u8 unknown_0c[10];         /* +0x0c */
    nation_t *nation;          /* +0x16 */
    city_t *city;              /* +0x1a */
    u8 unknown_1e[13];         /* +0x1e */
};

/* Non-player data, 68 bytes (DAT_00ae23c0, count DAT_00ae2368). */
struct non_player {
    s32 id;                    /* +0x00 */
    s16 unknown_04[5];         /* +0x04 */
    u8 unknown_0e[21];         /* +0x0e */
    staff_t *staff[8];         /* +0x23 */
    u8 unknown_43;             /* +0x43 */
};

/* Player data, 70 bytes (DAT_00ae23c4, count DAT_00ae236c). */
struct player {
    s32 id;                    /* +0x00 */
    u8 unknown_04;             /* +0x04 */
    s16 unknown_05[5];         /* +0x05 */
    u8 unknown_0f[55];         /* +0x0f */
};

/* Staff comp, 101 bytes (DAT_00ae23e0, count DAT_00ae2388). */
struct staff_comp {
    s32 id;                    /* +0x00 */
    char name[51];             /* +0x04 */
    u8 unknown_37[28];         /* +0x37 */
    continent_t *continent;    /* +0x53 */
    nation_t *nation;          /* +0x57 */
    colour_t *colours[2];      /* +0x5b */
    u8 unknown_63[2];          /* +0x63 */
};

/*
 * Club comp and nation comp, 107 bytes (DAT_00ae23d0, count DAT_00ae2378).
 * One array: the club comps, the nation comps (DAT_00ae23d8, count
 * DAT_00ae2380) and 127 reserve rows.
 */
struct club_comp {
    s32 id;                    /* +0x00 */
    char name[51];             /* +0x04 */
    u8 unknown_37;             /* +0x37 */
    char short_name[26];       /* +0x38 */
    u8 unknown_52;             /* +0x52 */
    u8 b53;                    /* +0x53 */
    u8 unknown_54[3];          /* +0x54 */
    u8 b57;                    /* +0x57 */
    u8 b58;                    /* +0x58 */
    continent_t *continent;    /* +0x59 */
    nation_t *nation;          /* +0x5d */
    colour_t *colours[2];      /* +0x61 */
    s16 level;                 /* +0x69 inference */
};

/* First, second and common name, 60 bytes (DAT_00ae23f0, f4, f8). */
struct db_name {
    char name[51];             /* +0x00 */
    s32 id;                    /* +0x33 */
    s32 nation_id;             /* +0x37 not resolved */
    u8 count;                  /* +0x3b inference: use count. 1 for the
                                  manager rows */
};

/* Staff history, 17 bytes (DAT_00ae23cc, count DAT_00ae2374). */
struct staff_history {
    s32 id;                    /* +0x00 */
    staff_t *staff;            /* +0x04 */
    u8 unknown_08[2];          /* +0x08 */
    club_t *club;              /* +0x0a */
    u8 unknown_0e[3];          /* +0x0e */
};

/* Staff comp history, 58 bytes (DAT_00ae23e4, count DAT_00ae238c). */
struct staff_comp_history {
    s32 id;                    /* +0x00 */
    u8 unknown_04[54];         /* +0x04 the id pass leaves it */
};

/*
 * Comp history, 26 bytes (DAT_00ae23d4, count DAT_00ae237c). One array:
 * the club comp history and the nation comp history (DAT_00ae23dc, count
 * DAT_00ae2384).
 */
struct comp_history {
    s32 id;                    /* +0x00 */
    club_comp_t *comp;         /* +0x04 */
    u8 unknown_08[2];          /* +0x08 */
    club_t *clubs[4];          /* +0x0a */
};

/* City, 56 bytes (DAT_00ae23ac, count DAT_00ae2354). */
struct city {
    s32 id;                    /* +0x00 */
    char name[26];             /* +0x04 */
    u8 unknown_1e;             /* +0x1e */
    nation_t *nation;          /* +0x1f */
    u8 unknown_23[21];         /* +0x23 */
};

/* Preferences, 52 bytes (DAT_00ae23c8, count DAT_00ae2370), built. */
struct preferences {
    s32 id;                    /* +0x00 */
    club_t *clubs[6];          /* +0x04 inference: favourite clubs */
    staff_t *staff[6];         /* +0x1c inference: favourite staff */
};

/* The two ints at the start of index.dat (DAT_00ae2344, DAT_00ae2348).
   After a load: 127 reserve comp rows and 16 manager rows. */
extern s32 g_db_index_header[2];
extern db_index_entry_t g_db_index[DB_INDEX_ENTRIES];
extern s32 g_db_shared;                       /* DAT_00ae2340 */

extern club_t *g_clubs;                       /* DAT_00ae23b4 */
extern s32 g_club_count;                      /* DAT_00ae235c, with the national clubs */
extern club_t *g_nat_clubs;                   /* DAT_00ae23b8 */
extern s32 g_nat_club_count;                  /* DAT_00ae2360 */
extern colour_t *g_colours;                   /* DAT_00ae23ec */
extern s32 g_colour_count;                    /* DAT_00ae2394 */
extern continent_t *g_continents;             /* DAT_00ae23a4 */
extern s32 g_continent_count;                 /* DAT_00ae234c */
extern nation_t *g_nations;                   /* DAT_00ae23a8 */
extern s32 g_nation_count;                    /* DAT_00ae2350 */
extern stadium_t *g_stadiums;                 /* DAT_00ae23b0 */
extern s32 g_stadium_count;                   /* DAT_00ae2358 */
extern staff_t *g_staff;                      /* DAT_00ae23bc */
extern s32 g_staff_count;                     /* DAT_00ae2364 */
extern official_t *g_officials;               /* DAT_00ae23e8 */
extern s32 g_official_count;                  /* DAT_00ae2390 */
extern non_player_t *g_non_players;           /* DAT_00ae23c0 */
extern s32 g_non_player_count;                /* DAT_00ae2368 */
extern player_t *g_players;                   /* DAT_00ae23c4 */
extern s32 g_player_count;                    /* DAT_00ae236c */
extern staff_comp_t *g_staff_comps;           /* DAT_00ae23e0 */
extern s32 g_staff_comp_count;                /* DAT_00ae2388 */
extern club_comp_t *g_club_comps;             /* DAT_00ae23d0 */
extern s32 g_club_comp_count;                 /* DAT_00ae2378 */
extern club_comp_t *g_nation_comps;           /* DAT_00ae23d8 */
extern s32 g_nation_comp_count;               /* DAT_00ae2380 */
extern db_name_t *g_first_names;              /* DAT_00ae23f0 */
extern s32 g_first_name_count;                /* DAT_00ae2398 */
extern db_name_t *g_second_names;             /* DAT_00ae23f4 */
extern s32 g_second_name_count;               /* DAT_00ae239c */
extern db_name_t *g_common_names;             /* DAT_00ae23f8 */
extern s32 g_common_name_count;               /* DAT_00ae23a0 */
extern staff_history_t *g_staff_histories;    /* DAT_00ae23cc */
extern s32 g_staff_history_count;             /* DAT_00ae2374 */
extern staff_comp_history_t *g_staff_comp_histories; /* DAT_00ae23e4 */
extern s32 g_staff_comp_history_count;        /* DAT_00ae238c */
extern comp_history_t *g_comp_histories;      /* DAT_00ae23d4 */
extern s32 g_comp_history_count;              /* DAT_00ae237c */
extern comp_history_t *g_nation_comp_histories; /* DAT_00ae23dc */
extern s32 g_nation_comp_history_count;       /* DAT_00ae2384 */
extern city_t *g_cities;                      /* DAT_00ae23ac */
extern s32 g_city_count;                      /* DAT_00ae2354 */
extern preferences_t *g_preferences;          /* DAT_00ae23c8 */
extern s32 g_preference_count;                /* DAT_00ae2370 */

/*
 * The row of the first empty common name, which the staff pass gives a
 * staff row with no name (DAT_009b59b4, -1 at the start of the original).
 */
extern s32 g_db_empty_common_name;

/*
 * FUN_00524d50: FUN_00528540(dir, 1, 0), the load of a new game. Returns
 * false when a file is missing or short, a format is not one that the
 * shipped data has, or memory runs out. The original shows an error box and
 * ends for some of these. The steps, which the differential test also runs
 * one by one:
 *   database_read      the loader up to the call of the id pass
 *   database_resolve   FUN_005315a0
 *   database_staff_pass, database_national_clubs
 *   key_index_set      FUN_00601a40 (key_index.h)
 */
bool database_load(const char *data_dir);
bool database_read(const char *data_dir);
void database_resolve(void);
void database_staff_pass(void);
void database_national_clubs(void);

/*
 * Index.dat, the continent table and the nation table, with the continent
 * and the rivals of each nation linked as the id pass does. The other id
 * fields of a nation keep their ids. For the league tests.
 */
bool database_load_nations(const char *data_dir);

/* FUN_00528080: frees the tables and sets each count to 0. */
void database_free(void);

/*
 * The tables in the layout of the original, for the differential test.
 * database_table_info gives the rows, the record size and the pointer
 * fields of a table: up to `max` entries of {offset, count, target table}.
 * database_record_bytes writes one record as the original holds it, with
 * the row behind each pointer, -1 for NULL, and the id itself for a field
 * that holds an id.
 */
typedef struct {
    s32 offset;
    s32 count;
    s32 target;
} db_link_t;

s32 database_table_info(s32 table, s32 *rows, s32 *record_size, db_link_t *links, s32 max);
void database_record_bytes(s32 table, s32 row, u8 *out);

/*
 * For database.c and database_links.c only: FUN_00549f70 with weekday -1
 * for a valid day (1..31) and month (0..11), and the switch of
 * database_record_bytes to pointers after the id pass.
 */
db_date_t database_date(s32 day, s32 month, s32 year);
void database_links_done(void);

/* One record of the nation and continent tables, for the league tests. */
void database_nation_bytes(const nation_t *n, u8 out[DB_NATION_SIZE]);
void database_continent_bytes(const continent_t *c, u8 out[DB_CONTINENT_SIZE]);

#endif
