/*
 * Commands of the port driver for the progress box and the local part of
 * opcode 7 (tests/diff/cases_progress.py), on g_screen_stack with the screen
 * manager of the test scene.
 */
#include "port_progress.h"
#include "port_helpers.h"
#include "port_scene.h"
#include "progress_box.h"
#include "screen_stack.h"
#include <stdio.h>
#include <string.h>

static int g_pumps;

/* Stands in for the message pump, which the test replaces in the original. */
static bool count_pump(void)
{
    g_pumps++;
    return true;
}

bool progress_command(const char *cmd, const char *args)
{
    screen_stack_t *st = &g_screen_stack;
    scrman_t *sm = st->record.screen_manager;

    if (!strcmp(cmd, "pg_state")) {
        /* "pg_state records darken unknown_12f551" */
        st->record_count = (s16)next_long(&args);
        sm->darken_screen = (s32)next_long(&args);
        sm->unknown_12f551 = (s32)next_long(&args);
        g_screen_pump = count_pump;
        g_pumps = 0;
        scene_record_flips();
    } else if (!strcmp(cmd, "pg_dump")) {
        printf("%d %d %d\n", g_pumps, sm->darken_screen, sm->unknown_12f551);
        g_pumps = 0;
    } else if (!strcmp(cmd, "pg_box")) {
        /* "pg_box step total -hex" */
        long step = next_long(&args), total = next_long(&args);

        printf("%d\n", gui_progress_box(next_text(&args), (s32)step, (s32)total));
    } else if (!strcmp(cmd, "pg_message")) {
        /* "pg_message step total channel -hex" */
        long step = next_long(&args), total = next_long(&args);
        long channel = next_long(&args);

        screen_stack_progress(st, next_text(&args), (s32)step, (s32)total,
                              (s16)channel);
    } else {
        return false;
    }
    return true;
}
