/* Test runs: a save at a fixed game date, for the same-save check (tests/smoke/smoke_run.py
 * --same-save). Only the environment switches it on. Play is unchanged without it.
 *
 * RECOMP_SAVE_AFTER_DAYS=n: at the first human turn n or more game days after the first human turn,
 * the runtime saves the game as samesave.sav in the game folder of the original (the write goes to the
 * user folder, files.c) and ends the program with code 0.
 *
 * The human turn is FUN_005cde40 (game.cpp). The day loop FUN_005cccd0 calls it once per slice, after
 * the day's processing (docs/research/2026-10-14-assistant-and-days-inventory.md, 2a). The save is
 * FUN_0082e430(path, 0x16), the call of the Auto Save at 0x7617d8. The folder is the save folder of
 * the game options, [0xae3138] + 0xf, as the Auto Save uses it at 0x7617bb. */
#include "win.h"

#include <SDL.h>
#include <stdio.h>
#include <stdlib.h>

#define GAME_DATE 0xae2c90u        /* s16 day of the year, s16 year */
#define GAME_OPTIONS 0xae3138u     /* pointer to the game options; +0xf is the save folder */
#define SAVE_GAME 0x0082e430u      /* cdecl (path, flags). The meaning of the result is unknown: the
                                    * test compares the files, so the runtime only prints it. */

static int save_after = -2;        /* -2: not read yet, -1: off */
static int first_day = -1;

static int game_day(void)
{
    int day = (int16_t)rd16(g_mem, GAME_DATE), year = (int16_t)rd16(g_mem, GAME_DATE + 2);
    return year * 366 + day;       /* only differences of a few months are used */
}

void o_005cde40(void);

/* The human turn, cdecl (short *last date): 1 while the turn goes on. */
RT_WRAP(005cde40, human_turn)
{
    holiday_next_turn();
    if (picker_check_turn()) {     /* RECOMP_PICK_CHECK: the dump is written */
        if (rt_on_fatal) rt_on_fatal();
        SDL_Quit();
        exit(0);
    }
    if (save_after == -2) {
        const char *v = getenv("RECOMP_SAVE_AFTER_DAYS");
        save_after = v && *v ? atoi(v) : -1;
    }
    if (save_after >= 0) {
        int today = game_day();
        if (first_day < 0) first_day = today;
        if (today - first_day >= save_after) {
            char path[512];
            uint32_t options = rd32(g_mem, GAME_OPTIONS);
            snprintf(path, sizeof path, "%ssamesave.sav", (const char *)(g_mem + options + 0xf));
            uint32_t args[2] = {rt_alloc_str(path), 0x16};
            uint32_t r = rt_callback(SAVE_GAME, 2, args);
            fprintf(stderr, "recomp: saved %s after %d game days (result %u)\n", path, today - first_day, r);
            if (rt_on_fatal) rt_on_fatal();
            SDL_Quit();
            exit(0);
        }
    }
    o_005cde40();
}
