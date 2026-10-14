"""Deliberate breaks of the nation and league tables. Each one must fail a suite. See tools/breaks.py."""
SUITES = ["leagues_index", "leagues_records", "leagues_other_files", "leagues_keys", "leagues_table", "leagues_default", "leagues_order",
          "leagues_extra", "leagues_name_compare"]

DB = "src/cm3/database.c"
KI = "src/cm3/key_index.c"
KT = "src/cm3/key_ids_table.c"
KN = "src/cm3/key_nation.c"
SETUP = "src/cm3/setup.c"
UTILS = "src/si/utils.c"

# Equivalent breaks, left out because no test can see them:
# - "if (!f) return true;" in read_index: the load then fails at the first table, as before.
# - "North america" for "North America": the match ignores case.
# - A field of the nation or continent layout at a wrong offset: the port reads and writes the
#   record with the same layout. The database_layouts test of breaks/database.py catches it.

# name, file, text to find (once in the file), text to put in its place
BREAKS = [
    ("index: header", DB, "    g_db_index_header[1] = get_s32(header + 4);", "    g_db_index_header[1] = get_s32(header);"),
    ("index: count field", DB, "        entry->count = get_s32(e + 0x37);", "        entry->count = get_s32(e + 0x3b);"),
    ("index: extra byte", DB, "    if (ok && fread(&extra, 1, 1, f) == 1 && extra != 0) ok = false;",
     "    if (ok && fread(&extra, 1, 1, f) != 1) ok = false;"),
    ("load: nation format", DB, "    case DB_NATION:                                      /* FUN_0054d740 */\n", ""),
    ("load: offset", DB, "    ok = fseek(f, entry->offset, SEEK_SET) == 0 &&", "    ok = fseek(f, 0, SEEK_SET) == 0 &&"),
    ("continent: double", DB, "RAW(continent_t, 0xbe, unknown_be)", "RAW(continent_t, 0xb6, unknown_be)"),
    ("nation: second name", DB, "RAW(nation_t, 0x038, second_name)", "RAW(nation_t, 0x004, second_name)"),
    ("nation: city", DB, "PTRS(nation_t, 0x07a, city, DB_CITY)", "PTRS(nation_t, 0x07e, city, DB_CITY)"),
    ("nation: colours", DB, "PTRS(nation_t, 0x090, colours, DB_COLOUR)", "PTRS(nation_t, 0x094, colours, DB_COLOUR)"),
    ("nation: rival link", DB, "            n->rivals[k] = id < 0 || id >= g_nation_count ? NULL : &g_nations[id];",
     "            n->rivals[k] = id <= 0 || id >= g_nation_count ? NULL : &g_nations[id];"),
    ("nation: continent link", DB, "        n->continent = id < 0 || id >= g_continent_count ? NULL : &g_continents[id];",
     "        n->continent = id < 0 || id >= g_continent_count ? NULL : &g_continents[0];"),
    ("keys: case", KI, "        if (x >= 'A' && x <= 'Z') x += 0x20;\n", ""),
    ("keys: first row wins", KI, "        g_key_ids[e->slot] = row;", "        if (g_key_ids[e->slot] < 0) g_key_ids[e->slot] = row;"),
    ("keys: no reset", KI, "    for (s32 k = 0; k < KEY_NATION_COUNT; k++) *key_id(nation_globals[k]) = -1;\n", ""),
    ("keys: nation name", KT, "{\"Republic Of Ireland\", 89, -1, 0},", "{\"Ireland\", 89, -1, 0},"),
    ("table: setup", KN, "    {KEY_WALES, NO_KEY, 0x8383b0, 1},", "    {KEY_WALES, NO_KEY, 0x8383b0, 0},"),
    ("table: continent", KN, "    {NO_KEY, KEY_ASIA, 0x831ba0, 1},", "    {NO_KEY, KEY_OCEANIA, 0x831ba0, 1},"),
    ("table: no check", KN, "    if (!g_nations) return false;\n    memset", "    memset"),
    ("order: index rows", KN, "        if (a == 1) return b == 2 ? -1 : b == 0 ? 1 : 0;", "        if (a == 1) return b == 2 ? 1 : b == 0 ? -1 : 0;"),
    ("order: continents first", KN, "        if (!bc) return -1;\n", "        if (!bc) return 1;\n"),
    ("order: second name", KN, "        return utils_name_compare(an->second_name, bn->second_name, 1, 0);",
     "        return utils_name_compare(an->name, bn->name, 1, 0);"),
    ("order: case", KN, "    return utils_name_compare(an->name, bn->name, 1, 0);", "    return utils_name_compare(an->name, bn->name, 0, 0);"),
    ("default: clear", SETUP, "    for (s32 i = 0; i < g_nation_count; i++) g_nations[i].league_flags = 0;\n", ""),
    ("default: language", SETUP, "        KEY_SWEDEN, KEY_JAPAN, KEY_SOUTH_KOREA,", "        KEY_SWEDEN, KEY_SOUTH_KOREA, KEY_JAPAN,"),
    ("default: range", SETUP, "    if ((u32)g_language_index > 12) return false;", "    if (g_language_index > 12) return false;"),
    ("default: counters", SETUP, "    g_leagues_to_load = 1;\n    g_selected_nations = 1;", "    g_leagues_to_load = 1;\n    g_selected_nations = 2;"),
    ("extra: size", KN, "    if (buf && size < 20) return 0;", "    if (buf && size < 19) return 0;"),
    ("extra: no buffer", KN, "    if (!n) return 0;\n", "    if (!n || !buf) return 0;\n"),
    ("extra: Spain", KN, " || id == g_key_nations[KEY_SPAIN]) {", ") {"),
    ("extra: note", KN, "\"Conference Division<%s - COMMENT - English Conference>\"", "\"Conference Division%s - COMMENT\""),
    ("compare: accents", UTILS, "    bool map = g_language_index != 10;", "    bool map = false;"),
    ("compare: language 10", UTILS, "    bool map = g_language_index != 10;", "    bool map = true;"),
    ("compare: lower case", UTILS, "                x = g_name_map[x][2];", "                x = g_name_map[x][1];"),
    ("compare: limit", UTILS, "            if (max_len > 0 && i == max_len - 1) return 0;", "            if (max_len > 0 && i == max_len) return 0;"),
    ("compare: longer", UTILS, "    if (b[i] != '\\0') return -1;", "    if (b[i] != '\\0') return 1;"),
    ("compare: null", UTILS, "    if (!a) return b ? -1 : 0;", "    if (!a) return -1;"),
    ("compare: signed", UTILS, "            if (x != y) return x < y ? -1 : 1;", "            if (x != y) return (s8)x < (s8)y ? -1 : 1;"),
]
