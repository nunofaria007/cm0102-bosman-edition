"""Deliberate breaks of the database load. Each one must fail a suite. See tools/breaks.py."""
SUITES = ["database_layouts", "database_read", "database_resolve", "database_staff_pass", "database_keys", "database_synthetic"]

DB = "src/cm3/database.c"
LINKS = "src/cm3/database_links.c"
KI = "src/cm3/key_index.c"
KT = "src/cm3/key_ids_table.c"

# Equivalent breaks, left out because no test can see them:
# - "blank: bytes", no clear of +0x56..+0x5e of a new row: the port allocates the table cleared.
# - "resolve: staff prefs", a lower bound for the preferences of a staff row: the conversion gives
#   each row its own preferences row, always below the count.
# - "leave: second team", no clear of the manager of the second national team in FUN_0053c190: the
#   load calls it only for an extinct club, which has no nation, or for a job other than 5.
# - "leave: unlink", no clear of the club link in FUN_0053c190: both calls of the load clear it after.
# - "keys: reset", no -1 before the chains: each load starts from -1, and the same data matches the
#   same rows.
# - "keys: club comps", the 127 reserve rows in the club comp chain: their names are empty.

# name, file, text to find (once in the file), text to put in its place
BREAKS = [
    # The layouts
    ("layout: continent code", DB, "RAW(continent_t, 0x1f, code)", "RAW(continent_t, 0x1e, code)"),
    ("layout: nation flags", DB, "RAW(nation_t, 0x11c, league_flags)", "RAW(nation_t, 0x11d, league_flags)"),
    ("layout: club squad", DB, "PTRS(club_t, 0x0d7, squad, DB_STAFF)", "PTRS(club_t, 0x0d8, squad, DB_STAFF)"),
    # The loader
    ("index: tail", DB, "    tail = DB_MANAGER_ROWS - g_db_index_header[1];", "    tail = DB_MANAGER_ROWS;"),
    ("size: clubs", DB, "        case DB_NAT_CLUB: g_nat_club_count += n; g_club_count += n; break;",
     "        case DB_NAT_CLUB: g_nat_club_count += n; break;"),
    ("size: reserve staff", DB, "            g_staff_count = n + tail + DB_RESERVE_STAFF + DB_SPARE_ROWS;",
     "            g_staff_count = n + tail + DB_RESERVE_STAFF + DB_SPARE_ROWS - 1;"),
    ("size: non-players", DB, "        case DB_NON_PLAYER: g_non_player_count = n + tail + DB_RESERVE_STAFF; break;",
     "        case DB_NON_PLAYER: g_non_player_count = n + tail; break;"),
    ("size: players", DB, "        case DB_PLAYER: g_player_count = n + DB_SPARE_ROWS; break;",
     "        case DB_PLAYER: g_player_count = n; break;"),
    ("size: reserve comps", DB, "        case DB_CLUB_COMP: g_club_comp_count += n + DB_RESERVE_COMPS; break;",
     "        case DB_CLUB_COMP: g_club_comp_count += n; break;"),
    ("size: names", DB, "        case DB_SECOND_NAME: g_second_name_count += n + tail; break;",
     "        case DB_SECOND_NAME: g_second_name_count += n; break;"),
    ("size: nation comps", DB, "        case DB_NATION_COMP: g_club_comp_count += n; g_nation_comp_count += n; break;",
     "        case DB_NATION_COMP: g_nation_comp_count += n; break;"),
    ("size: comp history", DB, "            g_comp_history_count += n;\n            g_nation_comp_history_count += n;",
     "            g_nation_comp_history_count += n;"),
    ("alloc: nation comp base", DB,
     "    g_nation_comps = g_club_comps + (g_club_comp_count - g_nation_comp_count - DB_RESERVE_COMPS);",
     "    g_nation_comps = g_club_comps + (g_club_comp_count - g_nation_comp_count);"),
    ("alloc: spare rows", DB, "    g_player_count -= DB_SPARE_ROWS;\n", ""),
    ("reserve comps: id", DB, "        r->id = row;\n        r->name[0] = '\\0';", "        r->id = row + 1;\n        r->name[0] = '\\0';"),
    ("reserve comps: nation", DB, "        r->nation = DB_FROM_ID(-1);\n        r->colours[0]", "        r->nation = DB_FROM_ID(0);\n        r->colours[0]"),
    ("reserve comps: level", DB, "        r->level = 0;", "        r->level = 1;"),
    ("blank: date", DB, "    db_date_t d = {40, 1972, 1, 0};", "    db_date_t d = {40, 1972, 0, 0};"),
    ("blank: age", DB, "    s->age = 1971;", "    s->age = 1970;"),
    ("blank: national club", DB, "    s->national_club = DB_FROM_ID(-1);", "    s->national_club = DB_FROM_ID(0);"),
    ("reserve: common name", DB, "        s->common_name = DB_FROM_ID(0);", "        s->common_name = DB_FROM_ID(-1);"),
    ("reserve: b5f", DB, "        s->b5f = 6;", "        s->b5f = 0;"),
    ("reserve: row", DB, "        staff_t *s = &g_staff[g_staff_count - i - tail];", "        staff_t *s = &g_staff[g_staff_count - i - tail - 1];"),
    ("non-player: staff", DB, "    for (s32 k = 0; k < 8; k++) np->staff[k] = DB_FROM_ID(-1);",
     "    for (s32 k = 0; k < 7; k++) np->staff[k] = DB_FROM_ID(-1);"),
    ("manager: prefs", DB, "        s->prefs = DB_FROM_ID(g_preference_count - i);   /* the count is still 0 */",
     "        s->prefs = DB_FROM_ID(-1);"),
    ("manager: name count", DB, "    n->count = 1;", "    n->count = 0;"),
    ("manager: name nation", DB, "    n->nation_id = -1;", "    n->nation_id = 0;"),
    ("reader: reputation", DB, "            if (c->reputation == 0) c->reputation = 1;\n", ""),
    ("reader: club comp rows", DB,
     "            ok = read_records(f, table, e->count, e->format, table_rows(table));",
     "            ok = read_records(f, table, e->count - (table == DB_CLUB_COMP), e->format, table_rows(table));"),
    ("convert: national job", DB, "        dst->national_job = (s8)src[0x28];", "        dst->national_job = (s8)src[0x3d];"),
    ("convert: player", DB, "        dst->player = DB_FROM_ID(get_s32(src + 0x91));", "        dst->player = DB_FROM_ID(get_s32(src + 0x95));"),
    ("convert: prefs staff", DB, "            p->staff[k] = DB_FROM_ID(get_s32(src + 0x79 + 4 * k));",
     "            p->staff[k] = DB_FROM_ID(get_s32(src + 0x75 + 4 * k));"),
    ("convert: blank prefs", DB, "        p->id = g_preference_count - i;\n        for (s32 k = 0; k < 6; k++) {\n            p->clubs[k] = DB_FROM_ID(-1);\n            p->staff[k] = DB_FROM_ID(-1);\n        }\n    }\n    free(g_staff_file);",
     "    }\n    free(g_staff_file);"),
    ("header: reserve comps", DB, "    g_db_index_header[0] = DB_RESERVE_COMPS;                   /* 0x52c687 */",
     "    g_db_index_header[0] = 0;"),
    # The id pass
    ("resolve: last division bug", LINKS, "    } else if (id > g_club_comp_count) {\n        c->comp_060 = NULL;",
     "    } else if (id > g_club_comp_count) {\n        c->last_division = NULL;"),
    ("resolve: comp 060 order", LINKS, "    LINK(c->comp_060, g_club_comps, g_club_comp_count, ABOVE);\n", ""),
    ("resolve: squad bound", LINKS, "    LINK_ALL(c->squad, g_staff, g_staff_count, FROM);", "    LINK_ALL(c->squad, g_staff, g_staff_count, ANY);"),
    ("resolve: staff 09b", LINKS, "    LINK_ALL(c->staff_09b, g_staff, g_staff_count, ABOVE);\n", ""),
    ("resolve: nation city", LINKS, "    LINK(n->city, g_cities, g_city_count, ANY);\n", ""),
    ("resolve: officials", LINKS, "        LINK(o->city, g_cities, g_city_count, ANY);\n", ""),
    ("resolve: stadium bug", LINKS, "    } else if (id > g_stadium_count) {\n        s->city = NULL;",
     "    } else if (id > g_stadium_count) {\n        s->stadium_48 = NULL;"),
    ("resolve: preferences", LINKS, "        LINK_ALL(p->staff, g_staff, g_staff_count, ANY);\n", ""),
    ("resolve: non-players", LINKS, "        LINK_ALL(g_non_players[i].staff, g_staff, g_staff_count, FROM);",
     "        LINK_ALL(g_non_players[i].staff, g_staff, g_staff_count - 1, FROM);"),
    ("resolve: staff history", LINKS, "        LINK(h->club, g_clubs, g_club_count, ANY);\n", ""),
    ("resolve: comp history", LINKS, "        LINK(h->comp, g_club_comps, g_club_comp_count, ANY);\n", ""),
    ("resolve: club comps", LINKS, "        LINK(c->continent, g_continents, g_continent_count, ANY);\n", ""),
    ("resolve: staff comps", LINKS, "        LINK(c->nation, g_nations, g_nation_count, FROM);\n", ""),
    ("resolve: cities", LINKS, "        LINK(g_cities[i].nation, g_nations, g_nation_count, ANY);\n", ""),
    # The staff pass
    ("staff: skip managers", LINKS, "        if (s->id >= g_staff_count - 16 || s->b5f == 6) continue;",
     "        if (s->b5f == 6) continue;"),
    ("staff: extinct club", LINKS, "            database_leave_club(s->club, s, NULL, true, false);\n            s->club = NULL;\n        }\n        if ((s->b5e",
     "            s->club = NULL;\n        }\n        if ((s->b5e"),
    ("staff: youth bit", LINKS, "            s->b5e = (u8)((s->b5e & ~2) | 1);", "            s->b5e = (u8)(s->b5e & ~2);"),
    ("staff: b23", LINKS, "        if (s->b23 && !s->b22) s->b23 = 0;\n", ""),
    ("staff: contract date", LINKS, "            s->contract_end = database_date(1, 8, 1997);", "            s->contract_end = database_date(2, 8, 1997);"),
    ("staff: contract limit", LINKS, "date_before(s->contract_end, database_date(1, 0, 1998))", "date_before(s->contract_end, database_date(1, 0, 1997))"),
    ("staff: job 7", LINKS, "        if (s->club_job == 7) s->club_job = 6;", "        if (s->club_job == 7) s->club_job = 7;"),
    ("staff: national manager", LINKS, "            if (s->national_club) s->national_club->manager = NULL;\n", ""),
    ("staff: nation from club", LINKS, "            if (s->club) s->nation = s->club->nation;", "            if (0) s->nation = s->club->nation;"),
    ("staff: job kinds", LINKS, "{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 2, 2, 3, 2}", "{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 2, 2, 2, 2}"),
    ("staff: both kinds", LINKS, "        if (!s->non_player) *job = 11;", "        if (!s->non_player) *job = 0;"),
    ("staff: player squad", LINKS, "            if (s->club) clear_all(s->club->squad, 50, s);\n", ""),
    ("staff: names", LINKS, "        if (!s->second_name) s->second_name = empty_common_name();\n", ""),
    ("staff: national leave", LINKS, "            database_leave_club(s->national_club, s, NULL, true, false);\n            s->national_job = 0;",
     "            s->national_job = 0;"),
    ("staff: national checks", LINKS, "        if (s->national_job > 0 && (s->national_job <= 13 || s->national_job == 15) &&",
     "        if (s->national_job > 0 && s->national_job <= 13 &&"),
    ("leave: assistant", LINKS, "    case 6: case 7:\n        if (c->assistant != s) return false;\n        c->assistant = NULL;",
     "    case 6: case 7:\n        if (c->assistant != s) return false;"),
    ("leave: coaches", LINKS, "    case 8:\n        if (!clear_in(c->coaches, 5, s)) return false;", "    case 8:\n        if (!clear_in(c->coaches, 4, s)) return false;"),
    ("youth: national teams", LINKS, "    if (!c || c->id >= g_club_count - g_nation_count) return false;", "    if (!c) return false;"),
    ("national clubs: half", LINKS, "reputation / 2);", "reputation / 3);"),
    ("national clubs: range", LINKS, "    for (s32 i = g_club_count - g_nation_count; i < g_club_count; i++) {",
     "    for (s32 i = g_club_count - g_nation_count + 1; i < g_club_count; i++) {"),
    ("national clubs: reserve", LINKS, "!strcmp(c->division->name, \"Reserve\")", "!strcmp(c->division->name, \"Reserves\")"),
    # The key ids
    ("keys: chains", KI, "        if (e->chain == done || !same_name(name, e->name)) continue;",
     "        if (done >= 0 || !same_name(name, e->name)) continue;"),
    ("keys: second id", KI, "        if (e->slot2 >= 0) g_key_ids[e->slot2] = row;\n", ""),
    ("keys: colours", KI, "    MATCH_ROWS(g_key_colour_chain, g_colours, g_colour_count);         /* 0x60386c */\n", ""),
    ("keys: staff comps", KI, "    MATCH_ROWS(g_key_staff_comp_chain, g_staff_comps, g_staff_comp_count);     /* 0x604347 */\n", ""),
    ("keys: clubs", KI, "    MATCH_ROWS(g_key_club_chain, g_clubs, g_club_count);               /* 0x604378 */\n", ""),
    ("keys: order", KI, "    MATCH_ROWS(g_key_continent_chain, g_continents, g_continent_count); /* 0x604278 */\n    MATCH_ROWS(g_key_staff",
     "    MATCH_ROWS(g_key_staff"),
]
