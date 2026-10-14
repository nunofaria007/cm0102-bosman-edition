/*
 * Commands of the port driver for the start screen (tests/diff/cases_start.py):
 * the picture table, and the start screen and the "Please Confirm" screen on
 * g_screen_stack with the screen manager of the test scene.
 */
#include "port_start.h"
#include "background.h"
#include "file_screens.h"
#include "port_helpers.h"
#include "screen_stack.h"
#include "setup.h"
#include "utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int fn_id(void (*fn)(void))
{
    if (fn == setup_build) return 1;
    if (fn == confirm_build) return 2;
    return fn ? 9 : 0;
}

static int input_id(s32 (*fn)(s32))
{
    if (fn == setup_input) return 1;
    if (fn == confirm_input) return 2;
    return fn ? 9 : 0;
}

static int position(const screen_node_t *wanted)
{
    int i = 0;

    if (!wanted) return -1;
    for (const screen_node_t *n = g_screen_stack.record.first; n; n = n->next) {
        if (n == wanted) return i;
        i++;
    }
    return -2;
}

/* The record, then a line for each screen with its functions, flag, title and slots. */
static void dump(void)
{
    const screen_record_t *rec = &g_screen_stack.record;
    int count = 0;

    for (const screen_node_t *n = rec->first; n; n = n->next) count++;
    printf("%d %d %d %d %d %d %d %d %d\n", rec->flagged, rec->count,
           rec->rebuild, rec->away, rec->done, position(rec->first),
           position(rec->last), position(rec->current), count);
    for (const screen_node_t *n = rec->first; n; n = n->next) {
        printf("%d %d %u ", fn_id(n->build), input_id(n->input), n->flag);
        print_hex(n->title);
        for (int i = 0; i < SCREEN_SLOTS; i++) {
            if (n->slots[i].owned) {
                printf(" s");
                print_hex((const char *)n->slots[i].value);
            } else {
                printf(" %lu", (unsigned long)(u32)n->slots[i].value);
            }
        }
        printf("\n");
    }
}

/* The loop of a pop of the main loop: frees the last screen. */
static void dump_pics(void)
{
    for (int i = 0; i < PICS_SECTIONS; i++) {
        const pics_section_t *s = &g_pics[i];

        printf("%d %d", s->count, s->entries != NULL);
        for (int k = 0; k < s->count; k++) {
            const pics_entry_t *e = &s->entries[k];

            printf(" k");
            print_hex(e->key);
            printf(" %d %d", e->count, e->count2);
            for (int n = 0; n < e->count; n++) {
                printf(" a");
                print_hex(e->names[n]);
            }
            for (int n = 0; n < e->count2; n++) {
                printf(" b");
                print_hex(e->names2[n]);
            }
        }
        printf("\n");
    }
}

bool start_command(const char *cmd, const char *args)
{
    screen_stack_t *st = &g_screen_stack;

    if (!strcmp(cmd, "st_pics_load")) {
        char path[1024];

        if (sscanf(args, "%1023s", path) != 1) return false;
        pics_free();
        g_pictures_dir = strdup(path);
        pics_load();
    } else if (!strcmp(cmd, "st_pics")) {
        dump_pics();
    } else if (!strcmp(cmd, "st_push_start")) {
        setup_push_start_screen();
    } else if (!strcmp(cmd, "st_confirm_push")) {
        char *title = next_text(&args), *heading = next_text(&args);
        char *text = next_text(&args);
        long v[4];

        for (int i = 0; i < 4; i++) v[i] = next_long(&args);
        confirm_push(title, heading, text, (s32)v[0], (u16)v[1], (u16)v[2],
                     (s32)v[3]);
    } else if (!strcmp(cmd, "st_title")) {
        screen_stack_set_title(st, next_text(&args));
    } else if (!strcmp(cmd, "st_build")) {
        if (st->record.current) st->record.current->build();
    } else if (!strcmp(cmd, "st_input")) {
        long clicked = next_long(&args);

        printf("%d\n", st->record.current
                           ? st->record.current->input((s32)clicked) : -999);
    } else if (!strcmp(cmd, "st_player")) {
        st->record.player = (s16)next_long(&args);
    } else if (!strcmp(cmd, "st_slot0")) {
        screen_stack_set_slot(st, 0, (uintptr_t)next_long(&args), false);
    } else if (!strcmp(cmd, "st_after_pop")) {
        st->after_pop = (s32)next_long(&args);
    } else if (!strcmp(cmd, "st_dialog")) {
        printf("%d %d\n", g_dialog_code, g_dialog_button);
    } else if (!strcmp(cmd, "st_set_dialog")) {
        g_dialog_code = (s32)next_long(&args);
        g_dialog_button = (s32)next_long(&args);
    } else if (!strcmp(cmd, "st_pop")) {
        screen_stack_pop(&g_screen_stack);
    } else if (!strcmp(cmd, "st_back")) {
        /* The result -2 of the main loop: the screen before becomes current. */
        if (st->record.current && st->record.current->prev) {
            st->record.current = st->record.current->prev;
        }
    } else if (!strcmp(cmd, "st_dump")) {
        dump();
    } else {
        return false;
    }
    return true;
}
