/* The picker of the Assistant drop-down (#12, assistant.c). Research:
 * docs/research/2026-10-15-assistant-pick-inventory.md.
 *
 * The original picks a team only inside a match (0x6db500, 0x6dbc50): it needs the match object and
 * draws random numbers. So the runtime picks the team itself, from the game's own data, and draws no
 * random numbers. It then stores the team with the game's own calls (0x896fe0, 0x896a90, 0x897760), as
 * the original's pick does.
 *
 * The slots are those of the player's active tactic (tactic +0x12d, a position mask per slot), so the
 * formation stays. A player's fit for a slot is the lower of his rating for the position and his
 * rating for the side, from the position ratings of the player data (1 to 20). A player's value in a
 * slot is his current ability (player data +0x05, 1 to 200) times that fit, divided by 20 (Max, Q98,
 * 2026-10-15): positional fit and skill together, so a natural player of low ability does not beat a
 * much better player who is able in the position. The picker fills the slot and player pair of the
 * highest value first. A swap pass then improves the total.
 *
 * Candidates: the players of the squad (club +0xd7) whom the squad screen shows as able to play the next
 * match. 0x45cbe0 is the screen's own test (0x459b3d): not injured, not loaned out, not banned, not away
 * with a national team and eligible for the competition. A slot of a player who fails it shows red.
 * - best: all candidates.
 * - match_fit: the candidates with condition 90% or more. When too few, the fittest of the others join.
 * - under_21: first the candidates under 21. The empty slots then get the best older players.
 *
 * Goalkeeper slot: as in the original (0x6e2000, research section "scorer"), a player with a goalkeeper
 * rating below 10 (player data +0x0f) never takes a slot that allows a goalkeeper (position bit 0x1).
 * So no outfield player goes in goal. When no young keeper exists, the gap fill takes the best older one.
 * With no keeper at all, the slot stays empty.
 *
 * No daily update: the getters for condition (0x62c590), injury (0x62c400) and the squad test (0x45cbe0)
 * first run the daily fitness update 0x62c680, which draws random numbers and changes the player. During
 * the pick the runtime wraps 0x62c680 and skips it, so the pick only reads what the records hold. A
 * record that no screen has updated today shows the value of its last update. */
#include "win.h"

#include <stdlib.h>
#include <string.h>

#define HUMANS 0xb63c98u         /* this of 0x5fbd00 */
#define STORE 0xde1d08u          /* the stored selections: this of 0x896fe0, 0x896a90, 0x897760 */
#define FITNESS 0xb63d28u        /* this of 0x62c400 and 0x62c590 */
#define NATIONS 0xb74318u        /* this of 0x7757d0 */
#define FIXTURES 0xae2a58u       /* this of 0x5acbe0 */
#define TODAY 0xae2c90u          /* the game date: s16 day of year, s16 year; this of 0x54c5b0 */
#define AVAILABLE 0x45cbe0u      /* cdecl (staff, fixture, club, date): 0 when the player cannot play */
#define SQUAD 0xd7u              /* club: 50 staff pointers */
#define MANAGER 0xcfu            /* club: the manager staff record */
#define PLAYER_DATA 0x61u        /* staff: the player data, 0 for no player */
#define AGE 0x18u                /* staff: s16 age */
#define ABILITY 0x05u            /* player data: s16 current ability, 1 to 200 */
#define SLOTS 11
#define MAX_SUBS 9               /* the store holds 20 slots */
#define DEFAULT_SUBS 5           /* without a fixture in the next 30 days. Inference: league rules */
#define FIT 90                   /* condition in % for "most match-fit" */
#define NATURAL 15               /* the game's "natural" rating (0x5b7d20) */
#define ABLE 10

/* The positions of the substitutes. The original draws them at random (0x6dd108); the runtime uses a
 * fixed list: GK, D, M, M, F, then M, D, F, M. */
static const uint16_t sub_masks[MAX_SUBS] = {0x1, 0x4, 0x10, 0x10, 0x40, 0x10, 0x4, 0x40, 0x10};

/* Position bits of a mask and the rating byte of the player data for each (0x5b7d20, 0x6e2f20):
 * GK, sweeper, D, DM, M, AM, F. Sides: 0x80 left (+0x18), 0x800 right (+0x17), 0x100, 0x200, 0x400
 * centre (+0x19). */
static const struct { uint16_t bit; uint8_t at; } positions[] = {
    {0x1, 0x0f}, {0x2, 0x10}, {0x4, 0x11}, {0x8, 0x12}, {0x10, 0x13}, {0x20, 0x14}, {0x40, 0x15}};

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

/* Set during picker_pick: the wrap of 0x62c680 skips the daily fitness update. */
static int picking;

void o_0062c680(void);

/* The daily fitness update of one player, thiscall (staff), ret 4. It draws random numbers and changes
 * the player (0x62c680 pseudocode). */
RT_WRAP(0062c680, daily_update)
{
    if (picking) {
        rt_ret_stdcall(cpu.eax, 1);
        return;
    }
    o_0062c680();
}

typedef struct { uint32_t staff, data; int condition, age, ability; } Candidate;

static int by_condition(const void *a, const void *b)
{
    return ((const Candidate *)b)->condition - ((const Candidate *)a)->condition;
}

/* The fit of a player for a slot mask, 1 to 20: the lower of the position and the side rating. */
static int fit(const Candidate *c, uint16_t mask)
{
    int pos = 0, side = 20;
    for (unsigned i = 0; i < sizeof positions / sizeof *positions; i++)
        if (mask & positions[i].bit) {
            int r = rd8(g_mem, c->data + positions[i].at);
            if (r > pos) pos = r;
        }
    if (!(mask & 1)) {
        if (mask & 0x80) side = rd8(g_mem, c->data + 0x18);
        else if (mask & 0x800) side = rd8(g_mem, c->data + 0x17);
        else if (mask & 0x700) side = rd8(g_mem, c->data + 0x19);
    }
    return pos < side ? pos : side;
}

/* The value of a player in a slot: current ability times fit / 20, in twentieths so that no rounding
 * hides a difference. -1 when the player cannot take the slot: a goalkeeper slot needs a goalkeeper
 * rating of 10 or more. */
static int value(const Candidate *c, uint16_t mask)
{
    if ((mask & 1) && rd8(g_mem, c->data + 0x0f) < ABLE) return -1;
    return (c->ability > 0 ? c->ability : 1) * fit(c, mask);
}

/* Fills the empty slots of team (an index into c, -1 for empty) with the candidates whose use[] is set. */
static void fill(const Candidate *c, int n, const int *use, const uint16_t *masks, int slots, int *team)
{
    int taken[50] = {0};
    for (int s = 0; s < slots; s++)
        if (team[s] >= 0) taken[team[s]] = 1;
    /* The eleven first, then the substitutes: each time the best free pair of all. */
    for (int part = 0; part < 2; part++) {
        int lo = part ? SLOTS : 0, hi = part ? slots : SLOTS;
        for (;;) {
            int bs = -1, bp = -1, bv = -1;
            for (int s = lo; s < hi; s++) {
                if (team[s] >= 0) continue;
                for (int p = 0; p < n; p++) {
                    if (!use[p] || taken[p]) continue;
                    int v = value(&c[p], masks[s]);
                    if (v >= 0 && v > bv) { bv = v; bs = s; bp = p; }
                }
            }
            if (bs < 0) break;
            team[bs] = bp;
            taken[bp] = 1;
        }
        /* Swap two players of this part when the sum of their values grows. */
        for (int changed = 1, pass = 0; changed && pass < 20; pass++) {
            changed = 0;
            for (int a = lo; a < hi; a++)
                for (int b = a + 1; b < hi; b++) {
                    if (team[a] < 0 || team[b] < 0) continue;
                    int now = value(&c[team[a]], masks[a]) + value(&c[team[b]], masks[b]);
                    int va = value(&c[team[b]], masks[a]), vb = value(&c[team[a]], masks[b]);
                    if (va >= 0 && vb >= 0 && va + vb > now) { int t = team[a]; team[a] = team[b]; team[b] = t; changed = 1; }
                }
        }
    }
}

static int pick(uint32_t club, uint32_t team_index, int kind)
{
    uint32_t manager = rd32(g_mem, club + MANAGER);
    uint32_t m[1] = {manager};
    uint32_t tactic = manager ? call_this(0x5fbd00u, rd32(g_mem, HUMANS), 1, m) : 0;
    if (!tactic || kind < 1 || kind > 3) return 0;
    uint32_t fixture = team_index ? 0 : next_fixture(club);
    int subs = fixture ? (int)(rd8(g_mem, fixture + 0x4d) & 0xf) : DEFAULT_SUBS;
    if (subs > MAX_SUBS) subs = MAX_SUBS;
    int slots = SLOTS + subs;
    uint16_t masks[SLOTS + MAX_SUBS];
    for (int s = 0; s < slots; s++)
        masks[s] = s < SLOTS ? (uint16_t)rd16(g_mem, tactic + 0x12d + 2 * (uint32_t)s) : sub_masks[s - SLOTS];

    Candidate c[50];
    int n = 0;
    for (uint32_t i = 0; i < 50; i++) {
        uint32_t staff = rd32(g_mem, club + SQUAD + 4 * i);
        if (!staff || !rd32(g_mem, staff + PLAYER_DATA)) continue;
        uint32_t s1[1] = {staff}, s2[2] = {staff, 1};
        /* Without a fixture 0x45cbe0 answers yes at once, so test injury and national duty here too. */
        if ((call_this(0x62c400u, rd32(g_mem, FITNESS), 1, s1) & 0xff) == 2) continue;   /* al */
        uint32_t intl[5] = {staff, TODAY, club, 0, 0};
        if (call_this(0x7757d0u, rd32(g_mem, NATIONS), 5, intl)) continue;
        uint32_t av[4] = {staff, fixture, club, TODAY};
        if (fixture && !(call_cdecl(AVAILABLE, 4, av) & 0xff)) continue;
        c[n].staff = staff;
        c[n].data = rd32(g_mem, staff + PLAYER_DATA);
        c[n].condition = (int16_t)call_this(0x62c590u, rd32(g_mem, FITNESS), 2, s2);
        c[n].age = (int16_t)rd16(g_mem, staff + AGE);
        c[n].ability = (int16_t)rd16(g_mem, c[n].data + ABILITY);
        n++;
    }
    qsort(c, (size_t)n, sizeof *c, by_condition);

    int first[50], all[50], n_first = 0, gk = 0;
    for (int i = 0; i < n; i++) {
        all[i] = 1;
        if (kind == 1) first[i] = 1;
        else if (kind == 3) first[i] = c[i].age < 21;
        else {                    /* the fit players, at least enough for the team, and a goalkeeper */
            int keeper = rd8(g_mem, c[i].data + 0x0f) >= NATURAL;
            first[i] = c[i].condition >= FIT || n_first < slots || (keeper && !gk);
            if (first[i]) gk |= keeper;
        }
        n_first += first[i];
    }

    int team[SLOTS + MAX_SUBS];
    for (int s = 0; s < slots; s++) team[s] = -1;
    fill(c, n, first, masks, slots, team);
    fill(c, n, all, masks, slots, team);     /* gaps: the best of the others */

    /* Store it: drop the undo copy and clear (0x896fe0), then each slot and the captain. */
    uint32_t drop[4] = {club, team_index, 0, 1}, clear[4] = {club, team_index, 0, 0};
    uint32_t store = rd32(g_mem, STORE);
    call_this(0x896fe0u, store, 4, drop);
    call_this(0x896fe0u, store, 4, clear);
    int picked = 0, captain = -1, best = 0;
    for (int s = 0; s < slots; s++) {
        if (team[s] < 0) continue;
        uint32_t staff = c[team[s]].staff;
        uint32_t set[4] = {club, (uint32_t)s, staff, team_index};
        call_this(0x896a90u, store, 4, set);
        picked++;
        if (s < SLOTS) {          /* the captain: the best value of 0x6e1cf0, as 0x6e1940 takes */
            uint32_t k[3] = {staff, 0, club};
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

/* kind 1: best team, 2: most match-fit, 3: under-21. Returns the number of players picked. */
int picker_pick(uint32_t club, uint32_t team_index, int kind)
{
    picking = 1;
    int n = pick(club, team_index, kind);
    picking = 0;
    return n;
}
