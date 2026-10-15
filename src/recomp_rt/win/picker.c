/* The picker of the Assistant drop-down (#12, assistant.c). Research:
 * docs/research/2026-10-15-assistant-pick-inventory.md.
 *
 * The original picks a team only inside a match (0x6db500, 0x6dbc50): it needs the match object and
 * draws random numbers. So the runtime calls the assignment core of that pick, 0x6e2000, on its own.
 * It takes the candidates and the position of each slot, scores each player for each slot with the
 * original's scorer 0x6e2f20 (position ratings, condition and more), and gives the eleven and the
 * substitutes. It draws no random numbers itself. The positions come from the player's active tactic,
 * so the formation stays. The runtime then stores the team with the game's own calls (0x896fe0,
 * 0x896a90, 0x897760), as the original's pick does.
 *
 * Candidates: the players of the squad (club +0xd7) who are not injured (0x62c400 == 2), not away with
 * a national team (0x7757d0) and not banned for the next match (0x8cf5e0, when a fixture is found).
 * - best: all candidates.
 * - match_fit: the candidates with condition 90% or more. When too few, the fittest of the others join.
 * - under_21: first the candidates under 21. The empty slots then get the best older players. */
#include "win.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define HUMANS 0xb63c98u         /* this of 0x5fbd00 */
#define STORE 0xde1d08u          /* the stored selections: this of 0x896fe0, 0x896a90, 0x897760 */
#define FITNESS 0xb63d28u        /* this of 0x62c400 and 0x62c590 */
#define NATIONS 0xb74318u        /* this of 0x7757d0 */
#define RULES 0xde1f54u          /* this of 0x8cf5e0 */
#define FIXTURES 0xae2a58u       /* this of 0x5acbe0 */
#define TODAY 0xae2c90u          /* the game date: s16 day of year, s16 year; this of 0x54c5b0 */
#define SQUAD 0xd7u              /* club: 50 staff pointers */
#define MANAGER 0xcfu            /* club: the manager staff record */
#define PLAYER_DATA 0x61u        /* staff: the player data, 0 for no player */
#define AGE 0x18u                /* staff: s16 age */
#define GK_RATING 0x0fu          /* player data: goalkeeper rating, 1 to 20 */
#define SLOTS 11
#define MAX_SUBS 9               /* the store holds 20 slots */
#define DEFAULT_SUBS 5           /* without a fixture in the next 30 days. Inference: league rules */
#define FIT 90                   /* condition in % for "most match-fit" */
#define FLAGS (0xca0cau | 0x10000u)   /* the flags of the original's pick for a club (0x6dcda5), no
                                       * random terms (inventory, 0x6e2000 flags) */

/* The positions of the substitutes. The original draws them at random (0x6dd108); the runtime uses a
 * fixed list: GK, D, M, M, F, then M, D, F, M. */
static const uint16_t sub_masks[MAX_SUBS] = {0x1, 0x4, 0x10, 0x10, 0x40, 0x10, 0x4, 0x40, 0x10};

static uint32_t call_this(uint32_t fn, uint32_t self, int n, const uint32_t *args)
{
    Cpu saved = cpu;
    cpu.ecx = self;
    uint32_t r = rt_callback(fn, n, args);
    cpu = saved;
    return r;
}

static uint32_t call_cdecl(uint32_t fn, int n, const uint32_t *args)
{
    Cpu saved = cpu;
    uint32_t r = rt_callback(fn, n, args);
    cpu = saved;
    return r;
}

/* The first fixture of club (team 1 = first team) from today to today + 30 days, or 0. */
static uint32_t next_fixture(uint32_t club)
{
    uint32_t to = rt_alloc_zero(8), list = rt_alloc_zero(16);
    uint32_t a[2] = {to, 30};
    call_this(0x54c5b0u, TODAY, 2, a);
    uint32_t f[9] = {rd16(g_mem, TODAY), rd16(g_mem, TODAY + 2), rd16(g_mem, to), rd16(g_mem, to + 2),
                     rd32(g_mem, club), list, 0xffffffffu, 0xfffffffeu, 1};
    uint32_t fixture = 0;
    if ((int16_t)call_this(0x5acbe0u, rd32(g_mem, FIXTURES), 9, f) > 0) {
        wr32(g_mem, list + 0xa, rd32(g_mem, list + 2));
        fixture = call_this(0x68d1b0u, list, 0, NULL);
    }
    call_this(0x68d040u, list, 0, NULL);
    rt_free(list);
    rt_free(to);
    return fixture;
}

typedef struct { uint32_t staff; int condition, age; } Candidate;

static int by_condition(const void *a, const void *b)
{
    return ((const Candidate *)b)->condition - ((const Candidate *)a)->condition;
}

/* Runs 0x6e2000 for the slots of masks (n_main main slots, then n_sub substitutes) with the
 * candidates in pool. Writes the chosen staff pointer of each slot to out (0 = empty). */
static void assign(uint32_t club, uint32_t manager, uint32_t tactic, const uint32_t *pool, int n_pool,
                   const uint16_t *masks, const int *slot_of, int n_main, int n_sub, uint32_t *out)
{
    int n = n_main + n_sub;
    if (!n || !n_pool) return;
    uint32_t players = rt_alloc_zero(4 * 50), pos = rt_alloc_zero(2 * 20), res = rt_alloc_zero(4 * 20);
    uint32_t slot8 = rt_alloc_zero(8 * 20), slot1 = rt_alloc_zero(20), pos2 = rt_alloc_zero(2 * 20);
    for (int i = 0; i < n_pool && i < 50; i++) wr32(g_mem, players + 4 * (uint32_t)i, pool[i]);
    for (int i = 0; i < n; i++) {
        wr16(g_mem, pos + 2 * (uint32_t)i, masks[i]);
        int s = slot_of[i];
        if (s < SLOTS) {          /* the tactic's data of the slot (0x5b2ae0, 0x5b5270, 0x5b57c0) */
            wr16(g_mem, pos2 + 2 * (uint32_t)i, rd16(g_mem, tactic + 0x14e + 2 * (uint32_t)s));
            for (uint32_t k = 0; k < 8; k++)
                wr8(g_mem, slot8 + 8 * (uint32_t)i + k, rd8(g_mem, tactic + 0x188b + 8 * (uint32_t)s + k));
            wr8(g_mem, slot1 + (uint32_t)i, rd8(g_mem, tactic + 0x5b4 + (uint32_t)s));
        }
    }
    uint32_t a[15] = {players, (uint32_t)(n_pool < 50 ? n_pool : 50), pos, (uint32_t)n_main, (uint32_t)n_sub, res,
                      FLAGS, club, 0, manager, 0, slot8, slot1, rd32(g_mem, tactic + 0x584), pos2};
    call_cdecl(0x6e2000u, 15, a);
    for (int i = 0; i < n; i++) out[slot_of[i]] = rd32(g_mem, res + 4 * (uint32_t)i);
    rt_free(players); rt_free(pos); rt_free(res); rt_free(slot8); rt_free(slot1); rt_free(pos2);
}

/* Fills the empty slots of team (subs substitutes) from pool. */
static void fill(uint32_t club, uint32_t manager, uint32_t tactic, uint32_t *team, int subs, const uint32_t *pool,
                 int n_pool)
{
    uint16_t masks[SLOTS + MAX_SUBS];
    int slot_of[SLOTS + MAX_SUBS], n_main = 0, n_sub = 0;
    uint32_t free_pool[50];
    int n_free = 0;
    for (int i = 0; i < n_pool; i++) {
        int used = 0;
        for (int s = 0; s < SLOTS + subs; s++) used |= team[s] == pool[i];
        if (!used) free_pool[n_free++] = pool[i];
    }
    for (int s = 0; s < SLOTS; s++)
        if (!team[s]) { masks[n_main] = rd16(g_mem, tactic + 0x12d + 2 * (uint32_t)s); slot_of[n_main++] = s; }
    for (int s = SLOTS; s < SLOTS + subs; s++)
        if (!team[s]) { masks[n_main + n_sub] = sub_masks[s - SLOTS]; slot_of[n_main + n_sub++] = s; }
    assign(club, manager, tactic, free_pool, n_free, masks, slot_of, n_main, n_sub, team);
}

/* kind 1: best team, 2: most match-fit, 3: under-21. Returns the number of players picked. */
int picker_pick(uint32_t club, uint32_t team_index, int kind)
{
    uint32_t manager = rd32(g_mem, club + MANAGER);
    uint32_t m[1] = {manager};
    uint32_t tactic = manager ? call_this(0x5fbd00u, rd32(g_mem, HUMANS), 1, m) : 0;
    if (!tactic || kind < 1 || kind > 3) return 0;
    uint32_t fixture = team_index ? 0 : next_fixture(club);
    int subs = fixture ? (int)(rd8(g_mem, fixture + 0x4d) & 0xf) : DEFAULT_SUBS;
    if (subs > MAX_SUBS) subs = MAX_SUBS;

    Candidate c[50];
    int n = 0;
    for (uint32_t i = 0; i < 50; i++) {
        uint32_t staff = rd32(g_mem, club + SQUAD + 4 * i);
        if (!staff || !rd32(g_mem, staff + PLAYER_DATA)) continue;
        uint32_t s1[1] = {staff}, s2[2] = {staff, 1};
        if ((call_this(0x62c400u, rd32(g_mem, FITNESS), 1, s1) & 0xff) == 2) continue;   /* al */
        uint32_t intl[5] = {staff, TODAY, club, 0, 0};
        if (call_this(0x7757d0u, rd32(g_mem, NATIONS), 5, intl)) continue;
        uint32_t ban[2] = {staff, fixture};
        if (fixture && (call_this(0x8cf5e0u, rd32(g_mem, RULES), 2, ban) & 0xff)) continue;   /* al */
        c[n].staff = staff;
        c[n].condition = (int16_t)call_this(0x62c590u, rd32(g_mem, FITNESS), 2, s2);
        c[n].age = (int16_t)rd16(g_mem, staff + AGE);
        n++;
    }

    uint32_t all[50], first[50];
    int n_first = 0;
    qsort(c, (size_t)n, sizeof *c, by_condition);
    for (int i = 0; i < n; i++) all[i] = c[i].staff;
    if (kind == 1) {
        memcpy(first, all, sizeof all);
        n_first = n;
    } else if (kind == 2) {
        /* The fit players, at least enough for the team, and a goalkeeper. */
        int gk = 0;
        for (int i = 0; i < n; i++) {
            int keeper = rd8(g_mem, rd32(g_mem, c[i].staff + PLAYER_DATA) + GK_RATING) >= 15;
            if (c[i].condition >= FIT || n_first < SLOTS + subs || (keeper && !gk)) {
                first[n_first++] = c[i].staff;
                gk |= keeper;
            }
        }
    } else {
        for (int i = 0; i < n; i++)
            if (c[i].age < 21) first[n_first++] = c[i].staff;
    }

    uint32_t team[SLOTS + MAX_SUBS] = {0};
    fill(club, manager, tactic, team, subs, first, n_first);
    fill(club, manager, tactic, team, subs, all, n);     /* gaps: the best of the others */

    /* Store it: drop the undo copy and clear (0x896fe0), then each slot and the captain. */
    uint32_t drop[4] = {club, team_index, 0, 1}, clear[4] = {club, team_index, 0, 0};
    uint32_t store = rd32(g_mem, STORE);
    call_this(0x896fe0u, store, 4, drop);
    call_this(0x896fe0u, store, 4, clear);
    int picked = 0, captain = -1, best = 0;
    for (int s = 0; s < SLOTS + subs; s++) {
        if (!team[s]) continue;
        uint32_t set[4] = {club, (uint32_t)s, team[s], team_index};
        call_this(0x896a90u, store, 4, set);
        picked++;
        if (s < SLOTS) {          /* the captain: the best value of 0x6e1cf0, as 0x6e1940 takes */
            uint32_t k[3] = {team[s], 0, club};
            int v = (int16_t)call_cdecl(0x6e1cf0u, 3, k);   /* a s16 (0x6e19f1) */
            if (captain < 0 || v > best) { captain = s; best = v; }
        }
    }
    if (captain >= 0) {
        uint32_t cap[3] = {club, (uint32_t)captain, team_index};
        call_this(0x897760u, store, 3, cap);
    }
    return picked;
}
