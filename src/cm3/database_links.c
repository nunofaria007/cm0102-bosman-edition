#include "database.h"
#include "key_index.h"
#include <string.h>

/*
 * The id pass FUN_005315a0, the staff pass (0x52d20a..0x52d81c) and the
 * national clubs (0x52d82c..0x52d8f8) of the loader FUN_00528540, with the
 * club functions that they call. See database.h.
 */

s32 g_db_empty_common_name = -1;

/* Upper bound tests of the id pass. */
enum bound {
    ANY,          /* only a negative id gives NULL */
    ABOVE,        /* an id above the count gives NULL: the count itself is a pointer past the end */
    FROM          /* an id at or above the count gives NULL */
};

static bool too_high(s32 id, s32 count, enum bound bound)
{
    return (bound == ABOVE && id > count) || (bound == FROM && id >= count);
}

/* One id field: NULL for a negative or too high id, else the row. */
#define LINK(field, rows, count, bound)                                          \
    do {                                                                         \
        s32 id_ = DB_ID(field);                                                  \
        (field) = id_ < 0 || too_high(id_, count, bound) ? NULL : &(rows)[id_];  \
    } while (0)

#define LINK_ALL(array, rows, count, bound)                                      \
    for (size_t k_ = 0; k_ < sizeof(array) / sizeof((array)[0]); k_++)           \
        LINK((array)[k_], rows, count, bound)

/*
 * 0x5315c5: the club loop, over the clubs and the national clubs. The
 * squad, the scouts, the coaches, the physios, the directors, the manager,
 * the chairman, the assistant and the 20 selected players drop an id at or
 * above the staff count. The original logs these, and a squad id more than
 * 1,325 above the count.
 */
static void resolve_club(club_t *c)
{
    s32 id;

    LINK(c->nation, g_nations, g_nation_count, ABOVE);
    LINK(c->division, g_club_comps, g_club_comp_count, ABOVE);
    /* 0x531657: a too high id clears +0x60 and stays in +0x5b. +0x60 then
       holds 0, which its own step below makes a pointer to row 0. */
    id = DB_ID(c->last_division);
    if (id < 0) {
        c->last_division = NULL;
    } else if (id > g_club_comp_count) {
        c->comp_060 = NULL;
    } else {
        c->last_division = &g_club_comps[id];
    }
    LINK(c->comp_060, g_club_comps, g_club_comp_count, ABOVE);
    LINK(c->stadium, g_stadiums, g_stadium_count, ABOVE);
    LINK(c->stadium_06e, g_stadiums, g_stadium_count, ABOVE);
    LINK_ALL(c->colours, g_colours, g_colour_count, ABOVE);
    LINK_ALL(c->staff_09b, g_staff, g_staff_count, ABOVE);
    LINK_ALL(c->clubs_0b3, g_clubs, g_club_count, ABOVE);
    LINK_ALL(c->squad, g_staff, g_staff_count, FROM);
    LINK_ALL(c->scouts, g_staff, g_staff_count, FROM);
    LINK_ALL(c->coaches, g_staff, g_staff_count, FROM);
    LINK_ALL(c->physios, g_staff, g_staff_count, FROM);
    LINK_ALL(c->directors, g_staff, g_staff_count, FROM);
    LINK(c->manager, g_staff, g_staff_count, FROM);
    LINK(c->chairman, g_staff, g_staff_count, FROM);
    LINK(c->assistant, g_staff, g_staff_count, FROM);
    LINK_ALL(c->team, g_staff, g_staff_count, FROM);
}

/* 0x531f64 */
static void resolve_nation(nation_t *n)
{
    LINK(n->continent, g_continents, g_continent_count, ANY);
    LINK(n->city, g_cities, g_city_count, ANY);
    LINK(n->stadium, g_stadiums, g_stadium_count, ANY);
    LINK_ALL(n->colours, g_colours, g_colour_count, ANY);
    LINK_ALL(n->rivals, g_nations, g_nation_count, ANY);
}

/* 0x53228c: a too high near stadium clears the city and keeps its id. */
static void resolve_stadium(stadium_t *s)
{
    s32 id;

    LINK(s->city, g_cities, g_city_count, ABOVE);
    id = DB_ID(s->stadium_48);
    if (id < 0) {
        s->stadium_48 = NULL;
    } else if (id > g_stadium_count) {
        s->city = NULL;
    } else {
        s->stadium_48 = &g_stadiums[id];
    }
}

/*
 * 0x532718: the staff loop. A player, preferences or non-player id at or
 * above its count gives NULL after an error box (Database.cpp lines 0x18f6,
 * 0x1907, 0x1918), which the port leaves out.
 */
static void resolve_staff(staff_t *s)
{
    LINK(s->first_name, g_first_names, g_first_name_count, ANY);
    LINK(s->second_name, g_second_names, g_second_name_count, ANY);
    LINK(s->common_name, g_common_names, g_common_name_count, ANY);
    LINK(s->nation, g_nations, g_nation_count, ANY);
    LINK(s->club, g_clubs, g_club_count, ANY);
    LINK(s->nation2, g_nations, g_nation_count, ANY);
    LINK(s->national_club, g_clubs, g_club_count, ANY);
    LINK(s->player, g_players, g_player_count, FROM);
    LINK(s->prefs, g_preferences, g_preference_count, FROM);
    LINK(s->non_player, g_non_players, g_non_player_count, FROM);
}

/* FUN_005315a0(0), the id pass, in the order of the original. */
void database_resolve(void)
{
    for (s32 i = 0; i < g_club_count; i++) resolve_club(&g_clubs[i]);
    for (s32 i = 0; i < g_nation_count; i++) resolve_nation(&g_nations[i]);
    for (s32 i = 0; i < g_official_count; i++) {            /* 0x5321d8 */
        official_t *o = &g_officials[i];
        LINK(o->first_name, g_first_names, g_first_name_count, ANY);
        LINK(o->second_name, g_second_names, g_second_name_count, ANY);
        LINK(o->nation, g_nations, g_nation_count, ANY);
        LINK(o->city, g_cities, g_city_count, ANY);
    }
    for (s32 i = 0; i < g_stadium_count; i++) resolve_stadium(&g_stadiums[i]);
    for (s32 i = 0; i < g_preference_count; i++) {          /* 0x532314 */
        preferences_t *p = &g_preferences[i];
        LINK_ALL(p->clubs, g_clubs, g_club_count, ANY);
        LINK_ALL(p->staff, g_staff, g_staff_count, ANY);
    }
    for (s32 i = 0; i < g_non_player_count; i++) {          /* 0x532535 */
        LINK_ALL(g_non_players[i].staff, g_staff, g_staff_count, FROM);
    }
    for (s32 i = 0; i < g_staff_count; i++) resolve_staff(&g_staff[i]);
    for (s32 i = 0; i < g_staff_comp_count; i++) {          /* 0x532a56 */
        staff_comp_t *c = &g_staff_comps[i];
        LINK(c->continent, g_continents, g_continent_count, FROM);
        LINK(c->nation, g_nations, g_nation_count, FROM);
        LINK_ALL(c->colours, g_colours, g_colour_count, FROM);
    }
    for (s32 i = 0; i < g_staff_history_count; i++) {       /* 0x532b5b */
        staff_history_t *h = &g_staff_histories[i];
        LINK(h->staff, g_staff, g_staff_count, ANY);
        LINK(h->club, g_clubs, g_club_count, ANY);
    }
    for (s32 i = 0; i < g_city_count; i++) {                /* 0x532bcb */
        LINK(g_cities[i].nation, g_nations, g_nation_count, ANY);
    }
    for (s32 i = 0; i < g_comp_history_count; i++) {        /* 0x532c11 */
        comp_history_t *h = &g_comp_histories[i];
        LINK(h->comp, g_club_comps, g_club_comp_count, ANY);
        LINK(h->clubs[0], g_clubs, g_club_count, ANY);
        LINK(h->clubs[1], g_clubs, g_club_count, ANY);
        LINK(h->clubs[2], g_clubs, g_club_count, ABOVE);
        LINK(h->clubs[3], g_clubs, g_club_count, ABOVE);
    }
    for (s32 i = 0; i < g_club_comp_count; i++) {           /* 0x532d27 */
        club_comp_t *c = &g_club_comps[i];
        LINK(c->nation, g_nations, g_nation_count, ANY);
        LINK(c->continent, g_continents, g_continent_count, ANY);
        LINK_ALL(c->colours, g_colours, g_colour_count, ANY);
    }
    database_links_done();
}

/* Club functions */

/* A national club: a row of the national club block at the end of the
   club array (0x53c25d). */
static bool is_national_club(const club_t *c)
{
    return c->id >= g_club_count - 2 * g_nation_count;
}

/* FUN_0053b3d0: the national team of a nation, a row of the first group of
   213 national clubs. */
static club_t *national_team(const nation_t *n)
{
    return &g_clubs[n->id - 2 * g_nation_count + g_club_count];
}

/* FUN_0053b4f0: the row of the second group of national clubs. */
static club_t *national_team_b(const nation_t *n)
{
    return &g_clubs[n->id - g_nation_count + g_club_count];
}

/* Finds a staff row in a list of a club and clears it. */
static bool clear_in(staff_t **list, s32 count, const staff_t *s)
{
    for (s32 i = 0; i < count; i++) {
        if (list[i] == s) {
            list[i] = NULL;
            return true;
        }
    }
    return false;
}

/*
 * FUN_0053c190(club, staff, date, unlink, as_player): takes a staff row out
 * of the list of the club that its job names (jump table 0x53c54c). Returns
 * false when the row is not in that list, and leaves its link. With a date
 * the end of the job gets it. With `unlink` the link of the staff row to
 * the club becomes NULL.
 */
bool database_leave_club(club_t *c, staff_t *s, const db_date_t *date, bool unlink, bool as_player)
{
    bool national;
    s8 job;

    if (!c || !s) return false;                              /* error box for no staff */
    national = is_national_club(c);
    if (date) {
        if (national) s->d29 = *date;
        else s->contract_end = *date;
    }
    job = national ? s->national_job : s->club_job;
    if (as_player) job = job == 11 || job == 12 || job == 13 || job == 15 ? 11 : 0;
    switch (job) {
    case 0:
        break;
    case 1:
        if (c->chairman != s) return false;
        c->chairman = NULL;
        break;
    case 2: case 3: case 4:
        if (!clear_in(c->directors, 3, s)) return false;
        break;
    case 5:
        if (c->manager != s) return false;
        c->manager = NULL;
        /* 0x53c306: a senior national team also clears the manager of its
           second team. The original reads through NULL for a club without
           a nation. */
        if (national && c->id < g_club_count - g_nation_count && c->nation) {
            club_t *b = national_team_b(c->nation);
            if (b->manager == s) b->manager = NULL;
        }
        break;
    case 6: case 7:
        if (c->assistant != s) return false;
        c->assistant = NULL;
        break;
    case 8:
        if (!clear_in(c->coaches, 5, s)) return false;
        break;
    case 9:
        if (!clear_in(c->scouts, 7, s)) return false;
        break;
    case 10:
        if (!clear_in(c->physios, 3, s)) return false;
        break;
    case 11:
        if (!clear_in(c->squad, 50, s)) return false;
        break;
    case 12:
        if (c->manager != s) return false;
        c->manager = NULL;
        if (!clear_in(c->squad, 50, s)) return false;
        break;
    case 13:
        if (c->assistant != s) return false;
        c->assistant = NULL;
        if (!clear_in(c->squad, 50, s)) return false;
        break;
    case 15:
        if (!clear_in(c->coaches, 5, s)) return false;
        if (!clear_in(c->squad, 50, s)) return false;
        break;
    default:
        return false;
    }
    if (unlink) {
        if (national) s->national_club = NULL;
        else s->club = NULL;
    }
    return true;
}

/*
 * FUN_00540a50(club, unused, need_league) as far as the load reaches it.
 * The load calls it with need_league 1, and a club read from the files has
 * +0x244 = 0, so it returns at 0x540aed. Left out: 0x540b1b..0x541e5f,
 * where a club of Spain, Germany or Portugal is looked up in about 500 key
 * club ids. The port returns 0 there.
 */
static bool club_check_540a50(const club_t *c, bool need_league)
{
    const nation_t *n;
    s32 id;

    if (!c || !c->nation) return false;
    if (need_league && !c->in_league) return false;
    n = c->nation;
    id = n->id;
    if (id != g_key_nations[KEY_SPAIN] && id != g_key_nations[KEY_GERMANY] &&
        id != g_key_nations[KEY_PORTUGAL]) {
        return false;
    }
    return false;
}

/*
 * FUN_00542740: false for a national team, for a club that
 * FUN_00540a50(club, 0, 1) accepts, and for a club of some key nations and
 * continents. Inference: whether the club can have a youth team.
 */
static bool club_check_542740(const club_t *c)
{
    const nation_t *n;

    if (!c || c->id >= g_club_count - g_nation_count) return false;
    if (club_check_540a50(c, true)) return false;
    n = c->nation;
    if (c->id >= g_club_count - 2 * g_nation_count && n) {     /* 0x542804 */
        if (n->id == g_key_nations[KEY_SPAIN] || n->id == g_key_nations[KEY_NORWAY] ||
            n->id == g_key_nations[KEY_ITALY] || n->id == g_key_nations[KEY_BRAZIL] ||
            n->id == g_key_nations[KEY_ARGENTINA]) {
            return false;
        }
        if (!n->continent) return true;
        return n->continent->id != g_key_continents[KEY_OCEANIA] &&
               n->continent->id != g_key_continents[KEY_AFRICA];
    }
    if (!n) return true;                                       /* 0x542860 */
    return n->id != g_key_nations[KEY_SPAIN] && n->id != g_key_nations[KEY_GERMANY] &&
           n->id != g_key_nations[KEY_PORTUGAL];
}

/* The staff pass */

/*
 * FUN_0090cfc0, the random generator, belongs to the dates and random
 * numbers slice. With the shipped data the staff pass never draws a number
 * (the differential test checks it). Until the slices merge, this stands in
 * for it.
 */
static s32 random_below(s32 n)
{
    (void)n;
    return 0;
}

/*
 * The common name that a staff row with no name gets (0x52d533): the first
 * empty common name, cached in g_db_empty_common_name. With no empty common
 * name, a random row, which is not cached.
 */
static db_name_t *empty_common_name(void)
{
    s32 row = g_db_empty_common_name;

    if (row == -1) {
        for (row = 0; row < g_common_name_count; row++) {
            if (g_common_names[row].name[0] == '\0') break;
        }
        if (row < g_common_name_count) g_db_empty_common_name = row;
        else row = random_below(g_common_name_count);
    }
    return &g_common_names[row];
}

/* The kind of a job, table 0x52daa0 for jobs 1..15: 0 needs non-player
   data, 1 player data, 2 both, 3 nothing. -1 for other values. */
static s32 job_kind(s8 job)
{
    static const s8 kinds[15] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 2, 2, 3, 2};
    return job >= 1 && job <= 15 ? kinds[job - 1] : -1;
}

/* Clears every entry of a squad that holds the row. */
static void clear_all(staff_t **list, s32 count, const staff_t *s)
{
    for (s32 i = 0; i < count; i++) {
        if (list[i] == s) list[i] = NULL;
    }
}

/* The checks of a job against the data of the row (jump tables 0x52da90 and
   0x52dab0). A player job without player data takes the row out of the
   squad of its club, for both jobs. */
static void check_job(staff_t *s, s8 *job)
{
    switch (job_kind(*job)) {
    case 0:
        if (!s->non_player) *job = 0;
        break;
    case 1:
        if (!s->player) {
            *job = 0;
            if (s->club) clear_all(s->club->squad, 50, s);
        }
        break;
    case 2:
        if (!s->non_player) *job = 11;
        if (!s->player) *job = 0;
        break;
    default:
        break;
    }
}

/* True when date a is before date b: the year first, then the day. */
static bool date_before(db_date_t a, db_date_t b)
{
    return a.year < b.year || (a.year == b.year && a.day < b.day);
}

/*
 * 0x52d20a: the staff pass. It skips the last 16 rows by id and the
 * reserve rows. The log lines and FUN_0053dba0, which only builds names for
 * them, are left out.
 */
void database_staff_pass(void)
{
    for (s32 i = 0; i < g_staff_count; i++) {
        staff_t *s = &g_staff[i];

        if (s->id >= g_staff_count - 16 || s->b5f == 6) continue;
        if (s->club && !s->club->nation) {                    /* extinct club, 0x52d23c */
            database_leave_club(s->club, s, NULL, true, false);
            s->club = NULL;
        }
        if ((s->b5e & 2) && s->club && !club_check_542740(s->club)) {   /* 0x52d29e */
            s->b5e = (u8)((s->b5e & ~2) | 1);
        }
        if (s->b23 && !s->b22) s->b23 = 0;
        if (!s->club && !s->national_club && s->non_player &&  /* 0x52d2d1 */
            date_before(s->contract_end, database_date(1, 0, 1998))) {
            s->contract_end = database_date(1, 8, 1997);
        }
        if (s->club_job == 7) s->club_job = 6;                 /* 0x52d33f */
        if (s->national_job == 7) s->national_job = 6;
        if (s->national_job == 5 && !s->non_player) {          /* 0x52d353 */
            if (s->national_club) s->national_club->manager = NULL;
            s->national_job = 0;
            s->national_club = NULL;
        }
        if (s->national_club && s->national_job == 0) s->national_club = NULL;
        if (!s->nation) {                                      /* 0x52d37e */
            if (s->club) s->nation = s->club->nation;
            else if (s->national_club) s->nation = s->national_club->nation;
            else s->nation = &g_nations[random_below(g_nation_count)];
        }
        check_job(s, &s->club_job);                            /* 0x52d3fb */
        if (!s->first_name) s->first_name = empty_common_name();   /* 0x52d52e */
        if (!s->second_name) s->second_name = empty_common_name();
        if (!s->common_name) s->common_name = empty_common_name();
        check_job(s, &s->national_job);                        /* 0x52d641 */
        if (s->non_player && s->national_club && s->national_job != 5) {   /* 0x52d780 */
            database_leave_club(s->national_club, s, NULL, true, false);
            s->national_job = 0;
            s->national_club = NULL;
        }
        if (s->national_job > 0 && (s->national_job <= 13 || s->national_job == 15) &&
            (!s->nation || !s->national_club)) {               /* 0x52d7af */
            s->national_club = NULL;
            s->national_job = 0;
        }
    }
}

/*
 * 0x52d82c: the second group of national clubs gets half the reputation of
 * the national team of its nation, and a club whose division is the
 * "Reserve" competition gets none.
 */
void database_national_clubs(void)
{
    for (s32 i = g_club_count - g_nation_count; i < g_club_count; i++) {
        club_t *c = &g_clubs[i];
        if (c->nation) c->reputation = (s16)(national_team(c->nation)->reputation / 2);
    }
    for (s32 i = 0; i < g_club_count; i++) {                  /* 0x52d896 */
        club_t *c = &g_clubs[i];
        if (c->division && !strcmp(c->division->name, "Reserve")) c->division = NULL;
    }
}
