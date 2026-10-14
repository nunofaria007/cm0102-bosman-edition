"""Deliberate breaks of the nation and league tables. Each one must fail a suite. See tools/breaks.py."""
SUITES = ["leagues_index", "leagues_records", "leagues_other_files", "leagues_keys", "leagues_table", "leagues_default", "leagues_order",
          "leagues_extra", "leagues_name_compare"]

DB = "src/cm3/database.c"
KI = "src/cm3/key_index.c"
KN = "src/cm3/key_nation.c"
SETUP = "src/cm3/setup.c"
UTILS = "src/si/utils.c"

# Equivalent breaks, left out because no test can see them:
# - "if (!f) return true;" in read_index: the load then fails at the first table, as before.
# - "North america" for "North America": the match ignores case.

# name, file, text to find (once in the file), text to put in its place
BREAKS = [
    ("index: header", DB, "    g_db_index_header[1] = get_s32(header + 4);", "    g_db_index_header[1] = get_s32(header);"),
    ("index: count field", DB, "        entry->count = get_s32(e + 0x37);", "        entry->count = get_s32(e + 0x3b);"),
    ("index: extra byte", DB, "    if (ok && fread(&extra, 1, 1, f) == 1 && extra != 0) ok = false;",
     "    if (ok && fread(&extra, 1, 1, f) != 1) ok = false;"),
    ("load: nation format", DB, "    if (!nations || nations->format != 2) return false;",
     "    if (!nations || nations->format != 1) return false;"),
    ("load: offset", DB, "    if (raw && fseek(f, entry->offset, SEEK_SET) != 0) {", "    if (raw && fseek(f, 0, SEEK_SET) != 0) {"),
    ("continent: code", DB, "    memcpy(c->code, p + 0x1f, sizeof(c->code));", "    memcpy(c->code, p + 0x1e, sizeof(c->code));"),
    ("continent: double", DB, "    memcpy(&c->unknown_be, p + 0xbe, sizeof(c->unknown_be));", "    c->unknown_be = 0;"),
    ("nation: second name", DB, "    memcpy(n->second_name, p + 0x38, sizeof(n->second_name));",
     "    memcpy(n->second_name, p + 0x04, sizeof(n->second_name));"),
    ("nation: city", DB, "    n->city_id = get_s32(p + 0x7a);", "    n->city_id = get_s32(p + 0x7e);"),
    ("nation: colours", DB, "    for (s32 i = 0; i < 6; i++) n->colour_ids[i] = get_s32(p + 0x90 + 4 * i);",
     "    for (s32 i = 0; i < 5; i++) n->colour_ids[i] = get_s32(p + 0x90 + 4 * i);"),
    ("nation: flags", DB, "    n->league_flags = p[0x11c];", "    n->league_flags = 1;"),
    ("nation: rival link", DB, "    return id < 0 || id >= g_nation_count ? NULL : &g_nations[id];",
     "    return id <= 0 || id >= g_nation_count ? NULL : &g_nations[id];"),
    ("nation: continent link", DB, "    return id < 0 || id >= g_continent_count ? NULL : &g_continents[id];",
     "    return id < 0 || id >= g_continent_count ? NULL : &g_continents[0];"),
    ("keys: case", KI, "        if (x >= 'A' && x <= 'Z') x += 0x20;\n", ""),
    ("keys: first row wins", KI, "                g_key_nations[k] = row;\n                break;",
     "                if (g_key_nations[k] < 0) g_key_nations[k] = row;\n                break;"),
    ("keys: no reset", KI, "    for (s32 k = 0; k < KEY_NATION_COUNT; k++) g_key_nations[k] = -1;\n", ""),
    ("keys: nation name", KI, "\"Republic Of Ireland\"", "\"Ireland\""),
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
