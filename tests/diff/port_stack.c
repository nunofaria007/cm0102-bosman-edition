/*
 * Commands of the port driver for the screen stack. The test names screen
 * functions by number: fake functions here that record each call, and stubs
 * in the emulator of the original.
 */
#include "port_stack.h"
#include "screen_stack.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define FAKES 8

static screen_stack_t g_stack;
static int g_calls[256];
static int g_call_count;

static void record_call(int id)
{
    if (g_call_count < 256) g_calls[g_call_count++] = id;
}

#define FAKE(n)                                                   \
    static void fake##n(void) { record_call(n); }                 \
    static s32 fake_input##n(s32 clicked)                         \
    {                                                             \
        (void)clicked;                                            \
        record_call(100 + n);                                     \
        return 0;                                                 \
    }
FAKE(1) FAKE(2) FAKE(3) FAKE(4) FAKE(5) FAKE(6) FAKE(7) FAKE(8)

static const screen_fn g_fakes[FAKES + 1] = {
    NULL, fake1, fake2, fake3, fake4, fake5, fake6, fake7, fake8,
};
static const screen_input_fn g_fake_inputs[FAKES + 1] = {
    NULL, fake_input1, fake_input2, fake_input3, fake_input4,
    fake_input5, fake_input6, fake_input7, fake_input8,
};

static screen_fn fake(long id)
{
    return id >= 1 && id <= FAKES ? g_fakes[id] : NULL;
}

static screen_input_fn fake_input(long id)
{
    return id >= 101 && id <= 100 + FAKES ? g_fake_inputs[id - 100] : NULL;
}

static int fake_id(screen_fn fn)
{
    for (int i = 1; i <= FAKES; i++) {
        if (fn == g_fakes[i]) return i;
    }
    return 0;
}

static int input_id(screen_input_fn fn)
{
    for (int i = 1; i <= FAKES; i++) {
        if (fn == g_fake_inputs[i]) return 100 + i;
    }
    return 0;
}

static void print_hex(const char *text)
{
    printf("-");
    for (const char *c = text; *c; c++) printf("%02x", (unsigned char)*c);
}

/* Reads "-hex" into a static buffer. Returns NULL for "null". */
static const char *read_hex(const char *args)
{
    static char text[1024];
    size_t length = 0;
    unsigned value;

    while (*args == ' ') args++;
    if (*args != '-') return NULL;
    for (args++; sscanf(args, "%2x", &value) == 1; args += 2) {
        if (length == sizeof(text) - 1) break;
        text[length++] = (char)value;
    }
    text[length] = '\0';
    return text;
}

static int position(const screen_node_t *wanted)
{
    int i = 0;

    if (!wanted) return -1;
    for (const screen_node_t *n = g_stack.record.first; n; n = n->next, i++) {
        if (n == wanted) return i;
    }
    return -2;
}

/* The record, then one line for each screen from the first. */
static void dump(void)
{
    const screen_record_t *rec = &g_stack.record;
    int count = 0;

    for (const screen_node_t *n = rec->first; n; n = n->next) count++;
    printf("%d %d %d %d %d %d %d %d %d %d %d %d %d %d\n", rec->player,
           rec->key != NULL, rec->flagged, rec->count, rec->rebuild,
           rec->away, rec->done, rec->unknown_22, rec->unknown_24,
           g_stack.record_count, position(rec->first), position(rec->last),
           position(rec->current), count);
    for (const screen_node_t *n = rec->first; n; n = n->next) {
        printf("%d %d %d %d %u %d ", fake_id(n->build), fake_id(n->leave),
               input_id(n->input), fake_id(n->exit), n->flag,
               position(n->prev));
        print_hex(n->title);
        for (int i = 0; i < SCREEN_SLOTS; i++) {
            if (n->slots[i].owned) {
                printf(" s");
                print_hex((const char *)n->slots[i].value);
            } else {
                printf(" %lu", (unsigned long)n->slots[i].value);
            }
        }
        printf("\n");
    }
}

bool stack_command(const char *cmd, const char *args)
{
    long v[16] = {0};
    int n = sscanf(args, "%ld %ld %ld %ld %ld %ld %ld %ld %ld %ld %ld %ld",
                   &v[0], &v[1], &v[2], &v[3], &v[4], &v[5], &v[6], &v[7],
                   &v[8], &v[9], &v[10], &v[11]);

    if (!strcmp(cmd, "stack_init")) {
        screen_stack_init(&g_stack, NULL);
    } else if (!strcmp(cmd, "stack_default")) {
        static uintptr_t values[8];

        if (n < 6 || v[5] < 0 || v[5] > 6) return false;
        for (int i = 0; i < v[5]; i++) values[i] = (uintptr_t)v[6 + i];
        screen_stack_set_default(&g_stack, fake(v[0]), fake_input(v[1]),
                                 (u32)v[2], fake(v[3]), fake(v[4]), values,
                                 (s16)v[5]);
    } else if (!strcmp(cmd, "stack_push")) {
        printf("%d\n", screen_stack_push(&g_stack, fake(v[0]),
                                         fake_input(v[1]), (u32)v[2],
                                         fake(v[3]), fake(v[4])));
    } else if (!strcmp(cmd, "stack_record")) {
        /* "stack_record key rebuild away done": key 0 or not 0 */
        if (n != 4) return false;
        g_stack.record.key = v[0] ? &g_stack : NULL;
        g_stack.record.rebuild = (s32)v[1];
        g_stack.record.away = (s32)v[2];
        g_stack.record.done = (s32)v[3];
    } else if (!strcmp(cmd, "stack_remove")) {
        screen_stack_remove(&g_stack);
    } else if (!strcmp(cmd, "stack_free")) {
        screen_stack_free_all(&g_stack);
    } else if (!strcmp(cmd, "stack_slot_set")) {
        /* "stack_slot_set slot 1 -hex" copies a text. "slot 0 value" does not. */
        if (n < 2) return false;
        if (v[1]) {
            const char *hex = strchr(args, '-');
            const char *text = hex ? read_hex(hex) : NULL;

            screen_stack_set_slot(&g_stack, (s32)v[0], (uintptr_t)text, true);
        } else {
            screen_stack_set_slot(&g_stack, (s32)v[0], (uintptr_t)v[2], false);
        }
    } else if (!strcmp(cmd, "stack_slot")) {
        printf("%lu\n", (unsigned long)screen_stack_slot(&g_stack, (s32)v[0]));
    } else if (!strcmp(cmd, "stack_title_set")) {
        screen_stack_set_title(&g_stack, read_hex(args));
    } else if (!strcmp(cmd, "stack_title")) {
        print_hex(screen_stack_title(&g_stack));
        printf("\n");
    } else if (!strcmp(cmd, "stack_queries")) {
        printf("%d %d %d %d\n", screen_stack_at_end(&g_stack),
               screen_stack_cannot_go_back(&g_stack, 0),
               screen_stack_cannot_go_back(&g_stack, 1),
               screen_stack_flagged(&g_stack));
    } else if (!strcmp(cmd, "stack_dump")) {
        dump();
    } else if (!strcmp(cmd, "stack_calls")) {
        printf("%d", g_call_count);
        for (int i = 0; i < g_call_count; i++) printf(" %d", g_calls[i]);
        printf("\n");
        g_call_count = 0;
    } else {
        return false;
    }
    return true;
}
