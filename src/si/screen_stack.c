#include "screen_stack.h"
#include "gui_utils.h"
#include "scrman_input.h"
#include <time.h>
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
    /* The copy comes first, so a slot can take a copy of its own text. */
    char *text = value && copy ? malloc(strlen((const char *)value) + 1) : NULL;

    if (value && copy && !text) return;
    if (text) strcpy(text, (const char *)value);
    if (node->slots[slot].owned) {
        free((void *)node->slots[slot].value);
        node->slots[slot].owned = 0;
    }
    if (text) {
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

bool (*g_screen_pump)(void);

static void leave_current(screen_record_t *rec)
{
    if (rec->current && rec->current->leave) rec->current->leave();
}

/* Frees the last screen and makes the one before it current and last. */
static void drop_last(screen_record_t *rec)
{
    screen_node_t *node = rec->last;

    rec->current = rec->last = node->prev;
    rec->last->next = NULL;
    free_node(rec, node);
}

bool screen_stack_pop(screen_stack_t *st)
{
    screen_record_t *rec = &st->record;

    for (;;) {
        screen_node_t *node = rec->last;
        bool flagged = node->flag != 0;

        if (!node->prev) {
            rec->flagged = 0;
            return false;
        }
        drop_last(rec);
        if (flagged) {
            rec->flagged--;
            return true;
        }
    }
}

/*
 * -10: frees the chain of flagged screens that ends the stack. With no such
 * chain it frees nothing. Returns false when the chain starts at the first
 * screen: the result then becomes -5.
 */
static bool pop_chain(screen_record_t *rec)
{
    screen_node_t *node = rec->last;

    while (node && !(node->flag && (!node->prev || !node->prev->flag))) {
        node = node->prev;
    }
    if (!node) return true;
    if (node == rec->first) {
        rec->flagged = 0;
        return false;
    }
    while (rec->last != node->prev) {
        if (rec->last->flag) rec->flagged--;
        drop_last(rec);
    }
    return true;
}

/*
 * -5, -6, -8 and -13. Returns true when a pending flagged screen takes over.
 * With flagged screens but none that fits, the original also sets the result
 * to 0, so -6 then acts as -5 and the loop returns 0.
 */
static bool next_flagged(screen_record_t *rec, s32 *result)
{
    if (rec->flagged <= 0) return false;
    for (screen_node_t *node = rec->last; node; node = node->prev) {
        if (node->flag && (*result == SCREEN_CONTINUE_ANY || !node->exit)) {
            rec->current = node;
            rec->rebuild = 1;
            *result = 0;
            return true;
        }
    }
    *result = 0;
    return false;
}

s32 screen_stack_run(screen_stack_t *st)
{
    screen_record_t *rec = &st->record;
    scrman_t *sm = rec->screen_manager;
    s32 result = 0;

    sm->darken_screen = 0;
    rec->away = rec->done = rec->rebuild ? 0 : 1;
    if (rec->rebuild) rec->started = (s32)time(NULL);

    for (;;) {
        result = 0;
        if (rec->rebuild && rec->current) {
            g_left_held = g_right_held = 0;
            rec->rebuild = 0;
            scrman_reset(sm, true);
            sm->skip_draw = 1;
            rec->current->build();
            sm->skip_draw = 0;
            sm->darken_screen = 0;
            scrman_draw(sm, screen_stack_title(st), -1, -1, -1, -1);
        }
        if (g_screen_pump && !g_screen_pump()) {
            result = SCREEN_QUIT;
            break;
        }

        s16 clicked = rec->away ? -1 : scrman_poll(sm);

        /* The original shows an error box here (scrman.cpp 455). */
        if (!rec->current) break;
        if (!rec->away) {
            s16 id = screen_stack_clicked_id(st, clicked);

            if (st->hook && id == st->hook_id) {
                st->hook(0);
                result = SCREEN_REBUILD;
            } else {
                result = rec->current->input(clicked);
            }
        }

    act:
        switch (result) {
        case SCREEN_HOME:
            leave_current(rec);
            if (rec->first) {
                rec->current = rec->first;
                rec->rebuild = 1;
            }
            break;
        case SCREEN_BACK:
            leave_current(rec);
            if (rec->current->prev && !rec->current->flag) {
                rec->current = rec->current->prev;
                rec->rebuild = 1;
            }
            break;
        case SCREEN_FORWARD:
            leave_current(rec);
            if (rec->current->next) {
                rec->current = rec->current->next;
                rec->rebuild = 1;
            }
            break;
        case SCREEN_REBUILD:
        case SCREEN_REBUILD_11:
            rec->rebuild = 1;
            break;
        case SCREEN_POP:
        case SCREEN_POP_CHAIN:
            leave_current(rec);
            if (!(result == SCREEN_POP ? screen_stack_pop(st) : pop_chain(rec))) {
                result = SCREEN_CONTINUE;
                goto act;
            }
            st->after_pop = 1;
            result = rec->current->input(-1);
            st->after_pop = 0;
            if (result == 0) {
                rec->rebuild = 1;
                break;
            }
            goto act;
        case SCREEN_CONTINUE:
        case SCREEN_CONTINUE_AWAY:
        case SCREEN_CONTINUE_8:
        case SCREEN_CONTINUE_ANY:
            if (next_flagged(rec, &result)) break;
            if (result == SCREEN_CONTINUE_AWAY) {
                rec->done = 1;
                rec->away = 1;
            } else if (!rec->away) {
                rec->done = !rec->done;
            }
            rec->rebuild = 1;
            break;
        default:
            /*
             * The original redraws the whole screen after a click here
             * (FUN_00802d00). It also sets a field of unknown use.
             */
            if (clicked != -1 && !rec->rebuild) {
                sm->unknown_12f551 = 1;
                sm->darken_screen = 0;
                scrman_draw(sm, screen_stack_title(st), -1, -1, -1, -1);
            }
            break;
        }
        /* -12 also ends a network game in the original, and does nothing more here. */
        if (rec->done || result == SCREEN_QUIT) break;
    }

    for (screen_node_t *node = rec->first; node; node = node->next) {
        if (node->exit) node->exit();
    }
    leave_current(rec);
    scrman_reset(sm, true);
    rec->current = NULL;
    rec->rebuild = 0;
    screen_stack_free_all(st);
    sm->darken_screen = 1;
    return result == SCREEN_CONTINUE;
}

screen_stack_t g_screen_stack;

s16 screen_stack_clicked_id(const screen_stack_t *st, s16 clicked)
{
    const scrman_t *sm = st->record.screen_manager;
    s16 id = clicked != -1 ? (s16)sm->widgets[clicked].unknown_08 : 0;

    return id ? id : st->injected_id;
}

s16 screen_heading(screen_stack_t *st, u16 colour, u16 text_colour,
                   const char *text)
{
    return scrman_widget_create(st->record.screen_manager, 1, 100, 10, 790,
                                70, 0, 0, 0x30, colour, 0, 0xc, 7,
                                text_colour, text, 0, 0, 0, -1);
}

s16 screen_sub_heading(screen_stack_t *st, const char *text)
{
    return scrman_widget_create(st->record.screen_manager, 1, 100, 80, 790,
                                125, 0, 0, 2, 0, 0, 0xc, 6, GUI_COLOUR_YELLOW,
                                text, 0, 0, 0, -1);
}

s16 screen_cell_button(screen_stack_t *st, s16 area, s32 column, s32 row,
                       u32 style, u16 colour, u32 text_flags, s16 font,
                       u16 text_colour, const char *text, s32 id,
                       intptr_t data)
{
    return scrman_widget_create(st->record.screen_manager, 2, 0, 0, 0, 0,
                                column, row, style, colour, colour,
                                text_flags, font, text_colour, text, 0, id,
                                data, area);
}

s16 screen_cell_label(screen_stack_t *st, s16 area, s32 column, s32 row,
                      u32 style, u16 colour, u32 text_flags, s16 font,
                      u16 text_colour, const char *text)
{
    return scrman_widget_create(st->record.screen_manager, 1, 0, 0, 0, 0,
                                column, row, style, colour, 0, text_flags,
                                font, text_colour, text, 0, 0, 0, area);
}

s16 screen_free_button(screen_stack_t *st, s32 x1, s32 y1, s32 x2, s32 y2,
                       u32 style, u16 colour, u32 text_flags, s16 font,
                       u16 text_colour, const char *text, s32 id,
                       intptr_t data)
{
    return scrman_widget_create(st->record.screen_manager, 2, x1, y1, x2, y2,
                                0, 0, style, colour, colour, text_flags, font,
                                text_colour, text, 0, id, data, -1);
}
