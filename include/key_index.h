#ifndef KEY_INDEX_H
#define KEY_INDEX_H

#include "database.h"

/*
 * Key ids of index.cpp. The original keeps about 1,100 globals at
 * 0x9cf178..0x9d0524, one for each record that its code names, such as
 * England or Europe. FUN_00601a40 sets them all to -1, then matches each row
 * of a table by name (_stricmp, ASCII case only) against a fixed list and
 * stores the row in the global of the name. A later row with the same name
 * wins. The port has the 27 nations and the 6 continents that the league
 * table and "Select League(s)" read. The other globals wait for their spec.
 */

enum key_nation {
    KEY_ARGENTINA,        /* DAT_009cf218 */
    KEY_AUSTRALIA,        /* DAT_009cf224 */
    KEY_BELGIUM,          /* DAT_009cf244 */
    KEY_BRAZIL,           /* DAT_009cf260 */
    KEY_CROATIA,          /* DAT_009cf2ac */
    KEY_DENMARK,          /* DAT_009cf2c0 */
    KEY_ENGLAND,          /* DAT_009cf2e4 */
    KEY_FINLAND,          /* DAT_009cf2fc */
    KEY_FRANCE,           /* DAT_009cf300 */
    KEY_GERMANY,          /* DAT_009cf314 */
    KEY_GREECE,           /* DAT_009cf31c */
    KEY_HOLLAND,          /* DAT_009cf338 */
    KEY_IRELAND,          /* DAT_009cf2dc "Republic Of Ireland" */
    KEY_ITALY,            /* DAT_009cf360 */
    KEY_JAPAN,            /* DAT_009cf36c */
    KEY_NORTHERN_IRELAND, /* DAT_009cf3e4 */
    KEY_NORWAY,           /* DAT_009cf408 */
    KEY_POLAND,           /* DAT_009cf430 */
    KEY_PORTUGAL,         /* DAT_009cf434 */
    KEY_RUSSIA,           /* DAT_009cf448 */
    KEY_SCOTLAND,         /* DAT_009cf45c */
    KEY_SOUTH_KOREA,      /* DAT_009cf484 */
    KEY_SPAIN,            /* DAT_009cf488 */
    KEY_SWEDEN,           /* DAT_009cf4a8 */
    KEY_TURKEY,           /* DAT_009cf4dc */
    KEY_UNITED_STATES,    /* DAT_009cf4f8 */
    KEY_WALES,            /* DAT_009cf510 */
    KEY_NATION_COUNT
};

enum key_continent {
    KEY_AFRICA,           /* DAT_009cfa08 */
    KEY_ASIA,             /* DAT_009cfa0c */
    KEY_EUROPE,           /* DAT_009cfa10 */
    KEY_NORTH_AMERICA,    /* DAT_009cfa14 */
    KEY_OCEANIA,          /* DAT_009cfa18 */
    KEY_SOUTH_AMERICA,    /* DAT_009cfa1c */
    KEY_CONTINENT_COUNT
};

/* The row of each key record in its table, or -1 when no row has the name. */
extern s32 g_key_nations[KEY_NATION_COUNT];
extern s32 g_key_continents[KEY_CONTINENT_COUNT];

/* The names that the original matches, in the order of the enums. */
extern const char *const g_key_nation_names[KEY_NATION_COUNT];
extern const char *const g_key_continent_names[KEY_CONTINENT_COUNT];

/*
 * The nation loop (0x60420d, which calls FUN_0060d810 for each row) and the
 * continent loop (0x604278) of FUN_00601a40, over g_nations and
 * g_continents. Sets every key id to -1 first.
 */
void key_index_match(void);

/* The nation of a key id, or NULL when the id is -1. */
nation_t *key_nation(enum key_nation key);
continent_t *key_continent(enum key_continent key);

#endif
