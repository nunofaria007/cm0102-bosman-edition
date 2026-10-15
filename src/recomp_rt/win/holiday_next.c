/* Holiday until next match (#30): a left bar item that starts the original's holiday and ends it the
 * day before the next match of the player's club.
 *
 * The original holiday has no length: the "Go on holiday ?" screen has only settings, and the holiday
 * runs until the player clicks (Return from holiday). The runtime adds the item "Holiday until next
 * match" below Go on Holiday. A click shows the same confirm screen. Yes starts the holiday as usual,
 * through the setter 0x5fd2f0, and the runtime marks it. At each human turn of a marked holiday, the
 * runtime looks up the club's next unplayed fixture with the game's own search 0x5acbe0 (the one that
 * the squad screen's Next Match uses). When it is today or tomorrow, the runtime ends the holiday as
 * Return from holiday does: the setter with 0, then the saved team selection back when the setting "Use
 * current team selection when possible" is on (0x762612..0x76267b).
 * See docs/research/2026-10-15-holiday-next-match-inventory.md.
 *
 * The mark is runtime state, not in the saved game: after a load the holiday goes on as in the original.
 * Days on holiday pass as on any holiday. The end changes nothing but the flag, as a click would. */
#include "win.h"

#include <stdio.h>
#include <string.h>

extern const uint32_t rt_site_holiday_item;

#define HUMANS 0xb63c98u            /* the human manager object: +0 records, 0xa6ef bytes each */
#define RECORD_SIZE 0xa6efu
#define ON_HOLIDAY 0x9a01u          /* record: the holiday flag */
#define SETTINGS 0x9b23u            /* record: the holiday settings, 3 = use current team selection */
#define STAFF_COUNT 0xae2364u       /* humans are the last 16 staff ids */
#define PLAYER_MANAGER 0xb74c26u    /* per player slot, 0x300 bytes apart */
#define PLAYER_SLOT 0xb77c7au
#define GAME_DAY 0xae2c90u          /* s16 day of the year from 0, s16 year */
#define FIXTURES 0xae2a58u          /* the fixture store; +0x1130 s16 year, +0x112c next year block */
#define TACTICS 0xde1d08u
#define PLAYERS 0xae23bcu           /* player records, 0x6e bytes each, +0x39 club */
#define RESULT_PENDING 0xdd6be0u    /* confirm screens: code and button */
#define RESULT_BUTTON 0xdd6be4u

#define SET_HOLIDAY 0x5fd2f0u       /* thiscall HUMANS (value, manager or 0), ret 8 */
#define SAVED_SELECTION 0x5fed50u   /* thiscall HUMANS (manager): record + 0x9b28, 8 bytes per slot */
#define SELECTION_CLEAR 0x896fe0u   /* thiscall TACTICS (club, 0, 0, 0) */
#define SELECTION_SET 0x896a90u     /* thiscall TACTICS (club, slot, player, 0) */
#define FIXTURE_SEARCH 0x5acbe0u    /* thiscall FIXTURES, 9 args, returns the count; team flag 1 = first team */
#define LIST_FREE 0x68d040u         /* thiscall on the list: frees its nodes */
#define CONFIRM_PUSH 0x6b2d60u      /* pushes the "Go on holiday ?" screen */
#define MESSAGE_BOX 0x5e8290u       /* cdecl (title, text, 0) */
#define SIDE_BAR_DISPLAY 0xb74c4cu  /* per player slot: the display; widget ids at + 0xba966 + w * 0x18c */

#define HOLIDAY_ID 0x3ef            /* Go on Holiday */
#define NEXT_MATCH_ID 0x4b7         /* the new item; the original uses no such id */
#define SEARCH_DAYS 60

static uint32_t item_args[18];
static uint32_t item_display;
static int pending;                 /* the confirm of the new item is open */
static int marked[16];              /* the holiday of this human ends before the next match */

static uint32_t call_this(uint32_t fn, uint32_t self, int n, const uint32_t *args)
{
    Cpu saved = cpu;
    cpu.ecx = self;
    uint32_t r = rt_callback(fn, n, args);
    cpu = saved;
    return r;
}

static uint32_t current_manager(void)
{
    return rd32(g_mem, PLAYER_MANAGER + (uint32_t)(int16_t)rd16(g_mem, PLAYER_SLOT) * 0x300u);
}

/* The index 0..15 of a human manager, or -1. */
static int human_slot(uint32_t manager)
{
    if (!manager) manager = current_manager();
    if (!manager) return -1;
    int slot = (int)rd32(g_mem, manager) - (int)rd32(g_mem, STAFF_COUNT) + 16;
    return slot >= 0 && slot < 16 ? slot : -1;
}

static uint32_t record(int slot) { return rd32(g_mem, rd32(g_mem, HUMANS)) + (uint32_t)slot * RECORD_SIZE; }

static int days_in(int year) { return (year % 4 == 0 && year % 100 != 0) || year % 400 == 0 ? 366 : 365; }

/* Days from 1.1.2000 to day (from 0) of year. */
static int serial(int day, int year)
{
    int n = day;
    for (int y = 2000; y < year; y++) n += days_in(y);
    return n;
}

/* The next unplayed fixture of the club from today on, as a serial day, or -1. The search ends at the
 * last year the fixture store holds, so it never makes a new year block (0x5abf10). */
static int next_fixture(uint32_t club)
{
    int day = (int16_t)rd16(g_mem, GAME_DAY), year = (int16_t)rd16(g_mem, GAME_DAY + 2);
    int end_day = day + SEARCH_DAYS, end_year = year;
    while (end_day >= days_in(end_year)) end_day -= days_in(end_year++);
    int last = -1;
    uint32_t store = rd32(g_mem, FIXTURES);
    for (uint32_t b = store, n = 0; b && n < 100; b = rd32(g_mem, b + 0x112c), n++)
        if ((int16_t)rd16(g_mem, b + 0x1130) > last) last = (int16_t)rd16(g_mem, b + 0x1130);
    if (last < year) return -1;
    if (end_year > last) { end_year = last; end_day = days_in(last) - 1; }
    uint32_t list = rt_alloc_zero(16);
    uint32_t args[9] = {(uint32_t)day, (uint32_t)year, (uint32_t)end_day, (uint32_t)end_year, rd32(g_mem, club),
                        list, 0xffffffffu, 0xfffffffeu, 1};
    call_this(FIXTURE_SEARCH, store, 9, args);
    int found = -1;
    for (uint32_t node = rd32(g_mem, list + 2), n = 0; node && n < 1000; node = rd32(g_mem, node + 6), n++) {
        uint32_t f = rd32(g_mem, node);
        if (f && (int8_t)g_mem[f + 0x43] == -1) {
            found = serial((int16_t)rd16(g_mem, f + 0x2a), (int16_t)rd16(g_mem, f + 0x28));
            break;
        }
    }
    call_this(LIST_FREE, list, 0, NULL);
    rt_free(list);
    return found;
}

static uint32_t club_of(uint32_t manager)
{
    uint32_t club = rd32(g_mem, manager + 0x39);
    return club ? club : rd32(g_mem, manager + 0x24);
}

/* The Go on Holiday item: keep its arguments, before the wrap scales them. */
int holiday_widget_before(void)
{
    if (rd32(g_mem, cpu.esp) != rt_site_holiday_item) return 0;
    for (int i = 0; i < 18; i++) item_args[i] = rt_arg(i);
    item_display = cpu.ecx;
    return 1;
}

/* After Go on Holiday: the same item below it, with the new text and id. Greyed when it is greyed. */
void holiday_widget_after(void)
{
    static uint32_t text;
    if (!text) text = rt_alloc_str("Holiday until next match");
    uint32_t args[18];
    memcpy(args, item_args, sizeof args);
    args[13] = text;
    if (args[15] == HOLIDAY_ID) args[15] = NEXT_MATCH_ID;
    uint32_t eax = cpu.eax;
    call_this(0x55f990u, item_display, 18, args);
    cpu.eax = eax;
}

static void message(const char *text)
{
    Cpu saved = cpu;
    uint32_t args[3] = {rt_alloc_str("Holiday until next match"), rt_alloc_str(text), 0};
    rt_callback(MESSAGE_BOX, 3, args);
    cpu = saved;
    rt_free(args[0]);
    rt_free(args[1]);
}

void o_007615d0(void);
void o_005fd2f0(void);

/* The side bar input, cdecl (s16 widget). */
RT_WRAP(007615d0, side_bar_input)
{
    if (rd32(g_mem, RESULT_PENDING) == 8 && rd32(g_mem, RESULT_BUTTON) == 1) pending = 0;   /* No */
    int16_t w = (int16_t)rt_arg(0);
    if (w >= 0 && w < 1200) {
        uint32_t display = rd32(g_mem, SIDE_BAR_DISPLAY + (uint32_t)(int16_t)rd16(g_mem, PLAYER_SLOT) * 0x300u);
        int id = (int16_t)rd16(g_mem, display + 0xba966u + (uint32_t)w * 0x18cu);
        if (id == HOLIDAY_ID) pending = 0;
        if (id == NEXT_MATCH_ID) {
            uint32_t manager = current_manager();
            uint32_t club = manager ? club_of(manager) : 0;
            if (!club || next_fixture(club) < 0) {
                message("Your club has no match in the next 60 days.");
            } else {
                pending = 1;
                Cpu saved = cpu;
                rt_callback(CONFIRM_PUSH, 0, NULL);
                cpu = saved;
            }
            rt_ret_cdecl((uint32_t)-4);
            return;
        }
    }
    o_007615d0();
}

/* The holiday setter, thiscall (value, manager or 0). */
RT_WRAP(005fd2f0, holiday_set)
{
    uint32_t value = rt_arg(0);
    int slot = human_slot(rt_arg(1));
    o_005fd2f0();
    if (slot < 0) return;
    if (value && pending) {
        marked[slot] = 1;
        pending = 0;
        fprintf(stderr, "recomp: holiday until next match for human %d\n", slot);
    } else if (!value) marked[slot] = 0;
}

void holiday_next_turn(void)
{
    uint32_t manager = current_manager();
    int slot = human_slot(manager);
    if (slot < 0 || !marked[slot]) return;
    uint32_t rec = record(slot);
    if (!rd32(g_mem, rec + ON_HOLIDAY)) { marked[slot] = 0; return; }
    uint32_t club = club_of(manager);
    int next = club ? next_fixture(club) : -1;
    int today = serial((int16_t)rd16(g_mem, GAME_DAY), (int16_t)rd16(g_mem, GAME_DAY + 2));
    if (next < 0 || next > today + 1) return;
    uint32_t humans = rd32(g_mem, HUMANS);
    uint32_t off[2] = {0, manager};
    call_this(SET_HOLIDAY, humans, 2, off);                 /* clears marked[slot] */
    fprintf(stderr, "recomp: holiday ends before the next match (human %d, today %d, match %d)\n", slot, today, next);
    uint32_t team = rd32(g_mem, manager + 0x39);
    if (!team || !g_mem[rec + SETTINGS + 3]) return;
    uint32_t a1[1] = {manager};
    uint32_t saved = call_this(SAVED_SELECTION, humans, 1, a1);
    uint32_t tactics = rd32(g_mem, TACTICS);
    uint32_t clear[4] = {team, 0, 0, 0};
    call_this(SELECTION_CLEAR, tactics, 4, clear);
    for (uint32_t s = 0; s < 16; s++) {
        int32_t index = (int32_t)rd32(g_mem, saved + s * 8);
        if (index <= -1) continue;
        uint32_t player = rd32(g_mem, PLAYERS) + (uint32_t)index * 0x6eu;
        if (rd32(g_mem, player + 0x39) != team) continue;
        uint32_t set[4] = {team, s, player, 0};
        call_this(SELECTION_SET, tactics, 4, set);
    }
}
