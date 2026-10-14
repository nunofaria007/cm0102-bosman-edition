#include "screen_stack.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/*
 * The screen stack of scrman.cpp for one local player. Network packets,
 * other channels and other records of the original are left out.
 */

void screen_stack_init(screen_stack_t *st, scrman_t *sm)
{
    memset(st, 0, sizeof(*st));
    st->record_count = 1;
    st->record.rebuild = 1;
    st->record.unknown_22 = -1;
    st->record.screen_manager = sm;
}

static void free_slots(screen_node_t *node)
{
    for (s32 i = 0; i < SCREEN_SLOTS; i++) {
        if (node->slots[i].value && node->slots[i].owned) {
            free((void *)node->slots[i].value);
        }
    }
}

static void free_node(screen_record_t *rec, screen_node_t *node)
{
    free_slots(node);
    free(node);
    rec->count--;
}

/* A flagged screen with this build function becomes current again. */
static bool return_to_flagged(screen_record_t *rec, screen_fn build)
{
    for (screen_node_t *node = rec->first; node; node = node->next) {
        if (!node->flag || node->build != build) continue;

        screen_node_t *old = rec->current;

        for (; node; node = node->next) {
            if (node->flag) rec->current = node;
        }
        if (rec->current->build != build) rec->current = old;
        return true;
    }
    return false;
}

s32 screen_stack_push(screen_stack_t *st, screen_fn build,
                      screen_input_fn input, u32 flag, screen_fn leave,
                      screen_fn exit)
{
    screen_record_t *rec = &st->record;
    bool used_default = false;
    screen_node_t *node;

    if (!rec->key && !rec->first && st->default_build) {
        build = st->default_build;
        input = st->default_input;
        flag = st->default_flag;
        leave = st->default_leave;
        exit = st->default_exit;
        used_default = true;
    }
    /* The original shows an error box here (scrman.cpp 1261). */
    if (!build || !input) return 0;
    if (rec->current && flag && rec->first && return_to_flagged(rec, build)) {
        return 0;
    }

    /* The forward history goes. The original keeps its flagged count. */
    if (rec->current != rec->last) {
        node = rec->current->next;
        rec->last = rec->current;
        rec->last->next = NULL;
        while (node) {
            screen_node_t *next = node->next;

            free_node(rec, node);
            node = next;
        }
        rec->current = rec->last;
    }
    if (rec->count > SCREEN_MAX_NODES) {
        node = rec->first;
        if (node->flag) return 0;
        rec->first = node->next;
        rec->first->prev = NULL;
        free_node(rec, node);
    }
    if (rec->current && rec->current->leave) rec->current->leave();

    node = calloc(1, sizeof(*node));
    if (!node) return 0;
    node->build = build;
    node->leave = leave;
    node->input = input;
    node->exit = exit;
    node->flag = flag;
    /* No file has this name, so the screen draw shows DEFAULT_PIC.RGN. */
    strcpy(node->title, "\\");
    if (flag) rec->flagged++;
    if (rec->last) {
        rec->last->next = node;
        node->prev = rec->last;
    }
    rec->last = node;
    if (!rec->first) rec->first = node;
    rec->current = node;
    rec->count++;
    rec->away = 0;
    rec->unknown_22 = -1;
    rec->unknown_24 = 0;
    if (flag) rec->done = 0;
    rec->rebuild = 1;
    if (used_default) {
        for (s16 i = 0; i < st->default_count; i++) {
            screen_stack_set_slot(st, i, st->default_values[i], false);
        }
    }
    return 1;
}

void screen_stack_free_all(screen_stack_t *st)
{
    screen_record_t *rec = &st->record;

    for (screen_node_t *node = rec->first; node; node = rec->current) {
        rec->current = node->next;
        free_node(rec, node);
    }
    rec->first = rec->last = rec->current = NULL;
    rec->count = 0;
    rec->flagged = 0;
}

void screen_stack_remove(screen_stack_t *st)
{
    screen_record_t *rec = &st->record;
    scrman_t *sm = rec->screen_manager;

    if (rec->current && rec->current->leave) rec->current->leave();
    screen_stack_free_all(st);
    memset(rec, 0, sizeof(*rec));
    rec->rebuild = 1;
    rec->unknown_22 = -1;
    rec->screen_manager = sm;
    rec->started = -1;
    st->record_count = 1;
    screen_stack_push(st, st->default_build, st->default_input,
                      st->default_flag, st->default_leave, st->default_exit);
}

void screen_stack_set_default(screen_stack_t *st, screen_fn build,
                              screen_input_fn input, u32 flag,
                              screen_fn leave, screen_fn exit,
                              const uintptr_t *values, s16 count)
{
    st->default_build = build;
    st->default_input = input;
    st->default_flag = flag;
    st->default_leave = leave;
    st->default_exit = exit;
    st->default_values = values;
    st->default_count = count;
}

void screen_stack_set_slot(screen_stack_t *st, s32 slot, uintptr_t value,
                           bool copy)
{
    screen_node_t *node = st->record.current;

    /* The original ends the program (scrman.cpp 1754) or shows an error box. */
    if (!node || slot < 0 || slot >= SCREEN_SLOTS) return;
    if (node->slots[slot].owned) {
        free((void *)node->slots[slot].value);
        node->slots[slot].owned = 0;
    }
    if (value && copy) {
        char *text = malloc(strlen((const char *)value) + 1);

        if (!text) return;
        strcpy(text, (const char *)value);
        node->slots[slot].value = (uintptr_t)text;
        node->slots[slot].owned = 1;
    } else {
        node->slots[slot].value = value;
        node->slots[slot].owned = 0;
    }
}

uintptr_t screen_stack_slot(const screen_stack_t *st, s32 slot)
{
    const screen_node_t *node = st->record.current;

    if (!node || slot < 0 || slot >= SCREEN_SLOTS) return 0;
    return node->slots[slot].value;
}

void screen_stack_set_title(screen_stack_t *st, const char *title)
{
    screen_node_t *node = st->record.current;

    if (!node) return;
    snprintf(node->title, sizeof(node->title), "%s", title ? title : "");
}

char *screen_stack_title(screen_stack_t *st)
{
    static char empty[1]; /* DAT_00ad9c64 */

    return st->record.current ? st->record.current->title : empty;
}

s32 screen_stack_at_end(const screen_stack_t *st)
{
    /* The original ends the program when there is no screen. */
    if (!st->record.current) return 1;
    return st->record.current->next == NULL;
}

s32 screen_stack_cannot_go_back(const screen_stack_t *st, s32 any)
{
    const screen_node_t *node = st->record.current;

    if (node && node->prev && (any || !node->flag)) return 0;
    return 1;
}

s32 screen_stack_flagged(const screen_stack_t *st)
{
    return st->record.flagged;
}
