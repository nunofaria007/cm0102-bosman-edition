/* The holiday hold: faster days (#20) and repeatable test runs.
 *
 * On holiday, the side bar handler FUN_007615d0 keeps each screen for 2,000 ms of the millisecond
 * clock (0x762245..0x762259): it ends the screen only when the clock is at least the manager's
 * holiday stamp + 2000. The stamp is in the human manager record at +0x9a65. FUN_005fdd30 writes it
 * (at the match-day screen and at every input on holiday), FUN_005fdc30 reads it. 0x762245 is the
 * only reader. The hold counts time, not passes of the screen loop, and changes no game state
 * (docs/research/2026-10-14-assistant-and-days-inventory.md, 2b to 2d).
 *
 * faster_days = on: the reader gives the stamp minus 2000, so the hold ends in its first pass.
 *
 * RECOMP_FIXED_CLOCK (test runs only, kernel32.c): the stamp is a reading of the real clock, so it
 * made two saved games differ (human_manager.dat). With the variable, the runtime keeps the stamps
 * itself and writes 0 to the record. The hold works as before. */
#include "win.h"
#include "options.h"

#define HUMAN_BASE_ID_COUNT 0xae2364u      /* count of staff; humans are the last 16 rows */
#define HOLD_MS 2000u

static uint32_t stamps[16];

#define PLAYER_MANAGER 0xb74c26u           /* per player slot, 0x300 bytes apart: the manager */
#define PLAYER_SLOT 0xb77c7au              /* s16: the player slot that has the turn */

/* The index 0..15 of a human manager, or -1. A null manager is the one of the current player, as in
 * the original (0x5fdc30). */
static int human_slot(uint32_t manager)
{
    if (!manager) manager = rd32(g_mem, PLAYER_MANAGER + (uint32_t)(int16_t)rd16(g_mem, PLAYER_SLOT) * 0x300u);
    if (!manager) return -1;
    int slot = (int)rd32(g_mem, manager) - (int)rd32(g_mem, HUMAN_BASE_ID_COUNT) + 16;
    return slot >= 0 && slot < 16 ? slot : -1;
}

void o_005fdd30(void);
void o_005fdc30(void);

/* Writes the holiday stamp, thiscall (u32 ms, manager *); 1 when written. */
RT_WRAP(005fdd30, holiday_stamp_set)
{
    int slot = -1;
    if (win_fixed_clock()) {
        slot = human_slot(rt_arg(1));
        if (slot >= 0) stamps[slot] = rt_arg(0);
        wr32(g_mem, cpu.esp + 4, 0);
    }
    o_005fdd30();
}

/* Reads the holiday stamp, thiscall (manager *). */
RT_WRAP(005fdc30, holiday_stamp_get)
{
    uint32_t manager = rt_arg(0);
    o_005fdc30();
    if (win_fixed_clock()) {
        int slot = human_slot(manager);
        if (slot >= 0) cpu.eax = stamps[slot];
    }
    if (options_bool(OPT_FASTER_DAYS)) cpu.eax -= HOLD_MS;
}
