#ifndef SCREEN_STACK_H
#define SCREEN_STACK_H

#include "cm_types.h"
#include "scrman.h"
#include <stdint.h>

#define SCREEN_SLOTS      60
#define SCREEN_MAX_NODES  100
#define SCREEN_TITLE_SIZE 260

typedef void (*screen_fn)(void);
typedef s32 (*screen_input_fn)(s32 clicked);

/*
 * One screen on the screen stack, from scrman.cpp of the original. The
 * original node is 0x300 bytes. Each comment gives the original offset.
 */
typedef struct screen_node {
    screen_fn build;            /* 0x000 */
    screen_fn leave;            /* 0x004, before another screen takes over */
    screen_input_fn input;      /* 0x008, gets the clicked widget or -1 */
    screen_fn exit;             /* 0x00c, when the main loop ends */
    u32 flag;                   /* 0x010, a step of a chain the game forces */
    struct {
        uintptr_t value;        /* a number or a pointer */
        u32 owned;              /* 1: the stack frees the value */
    } slots[SCREEN_SLOTS];      /* 0x014 */
    struct screen_node *prev;   /* 0x1f4 */
    struct screen_node *next;   /* 0x1f8 */
    char title[SCREEN_TITLE_SIZE]; /* 0x1fc, the background file name */
} screen_node_t;

/* The record of the local player: channel 0, slot 0 at 0x7d4 of the stack. */
typedef struct {
    s16 player;                 /* 0x00, 0 = the local player */
    void *key;                  /* 0x02, the manager of this record */
    screen_node_t *first;       /* 0x06 */
    screen_node_t *last;        /* 0x0a */
    screen_node_t *current;     /* 0x0e */
    s16 flagged;                /* 0x12, nodes with the flag set */
    s16 count;                  /* 0x14, nodes */
    s32 rebuild;                /* 0x16, build on the next loop pass */
    s32 away;                   /* 0x1a, takes no input in this loop run */
    s32 done;                   /* 0x1e */
    s16 unknown_22;
    s32 unknown_24;
    scrman_t *screen_manager;   /* 0x28 */
    s32 started;                /* 0x2c, time() when the loop started */
} screen_record_t;

/*
 * The screen stack for one local player. The original object (global at
 * 0xb74450) holds 16 channels for network and hot-seat games.
 */
typedef struct {
    screen_record_t record;     /* 0x7d4 */
    screen_fn default_build;    /* 0x37d4, the screen of an empty stack */
    screen_input_fn default_input; /* 0x37d8 */
    u32 default_flag;           /* 0x37dc */
    screen_fn default_leave;    /* 0x37e0 */
    screen_fn default_exit;     /* 0x37e4 */
    const uintptr_t *default_values; /* 0x37e8, slot values of that screen */
    s16 default_count;          /* 0x37ec */
    s32 (*hook)(s32);           /* 0x37ee, runs when a click has hook_id */
    s32 hook_id;                /* 0x37f2 */
    s32 unknown_37f6;
    s16 record_count;           /* 0x380a, records of channel 0 */
    s32 after_pop;              /* 0x3834, 1 for the input call after a pop */
    s16 injected_id;            /* 0x26278e, the id of a click that code makes */
} screen_stack_t;

/*
 * The screen stack object of the original (0xb74450). The screens and the
 * helpers below work on it, as the original does.
 */
extern screen_stack_t g_screen_stack;

/* The part of FUN_007fc380 that one local player needs. */
void screen_stack_init(screen_stack_t *st, scrman_t *sm);

/*
 * FUN_007fe600. Puts a new screen on top and asks for a rebuild. Returns 0
 * when it pushes nothing: no build or input function, a flagged screen with
 * this build function already on the stack (it becomes current again), or a
 * full stack whose first screen is flagged. Calls the leave function of the
 * screen it covers. The forward history above the current screen goes.
 */
s32 screen_stack_push(screen_stack_t *st, screen_fn build,
                      screen_input_fn input, u32 flag, screen_fn leave,
                      screen_fn exit);

/*
 * FUN_007ff990 for the record of the local player. Calls the leave function,
 * frees every screen, then starts again with the default screen.
 */
void screen_stack_remove(screen_stack_t *st);

/* FUN_007fef10. Frees every screen of the record. */
void screen_stack_free_all(screen_stack_t *st);

/* FUN_007feb20. The screen that a push onto an empty stack gives. */
void screen_stack_set_default(screen_stack_t *st, screen_fn build,
                              screen_input_fn input, u32 flag,
                              screen_fn leave, screen_fn exit,
                              const uintptr_t *values, s16 count);

/*
 * FUN_007ff270. With `copy`, the slot keeps its own copy of the text in
 * `value`. A slot outside 0 to 59 is ignored, as is a stack with no screen
 * (the original ends the program there).
 */
void screen_stack_set_slot(screen_stack_t *st, s32 slot, uintptr_t value,
                           bool copy);

/* FUN_007ff010. 0 for a slot outside 0 to 59 or a stack with no screen. */
uintptr_t screen_stack_slot(const screen_stack_t *st, s32 slot);

/*
 * FUN_007feeb0. NULL gives an empty title. The original copies with no
 * limit. The port keeps 259 characters.
 */
void screen_stack_set_title(screen_stack_t *st, const char *title);

/* FUN_007fee80. "" when the stack has no screen. */
char *screen_stack_title(screen_stack_t *st);

/* FUN_007febb0. 1 when the current screen has no screen after it. */
s32 screen_stack_at_end(const screen_stack_t *st);

/*
 * FUN_007fec60. 0 when there is a screen to go back to: the current screen
 * has a previous one and `any` is set or the current screen has no flag.
 */
s32 screen_stack_cannot_go_back(const screen_stack_t *st, s32 any);

/* FUN_00803d50. Flagged screens of every record. */
s32 screen_stack_flagged(const screen_stack_t *st);

/*
 * The message pump of the original (FUN_0068d520): it sets the mouse globals
 * and the tick. The app sets it. It returns false when the window closes.
 */
extern bool (*g_screen_pump)(void);

/* Results of an input function. 0 and up keep the screen. */
#define SCREEN_HOME          -1
#define SCREEN_BACK          -2
#define SCREEN_FORWARD       -3
#define SCREEN_REBUILD       -4
#define SCREEN_CONTINUE      -5
#define SCREEN_CONTINUE_AWAY -6
#define SCREEN_QUIT          -7
#define SCREEN_CONTINUE_8    -8
#define SCREEN_POP           -9
#define SCREEN_POP_CHAIN     -10
#define SCREEN_REBUILD_11    -11
#define SCREEN_QUIT_12       -12
#define SCREEN_CONTINUE_ANY  -13

/*
 * FUN_007fc7e0 for one local player. Each pass builds and draws the current
 * screen when it needs it, pumps the events, polls the input, calls the input
 * function with the clicked widget or -1, and acts on the result. The loop
 * ends on SCREEN_QUIT, on a continue with no flagged screen left, or when the
 * window closes. Then it calls the exit function of every screen and frees
 * the stack. Returns 1 when the last result was SCREEN_CONTINUE. The original
 * shows "Processing, Please Wait ..." at the end. The port does not.
 */
s32 screen_stack_run(screen_stack_t *st);

/*
 * FUN_00468b40. The id of the clicked widget (widget +0x08, 16 bits), or
 * the injected id when no widget is clicked or the id is 0.
 */
s16 screen_stack_clicked_id(const screen_stack_t *st, s16 clicked);

/*
 * Widget helpers of scrman.cpp. Each one creates a widget in the screen
 * manager of the record and returns its index. The original passes each
 * colour and font as 32 bits, and the widget keeps 16 of them.
 */

/* FUN_00803f30. Font 7, centred, box (100, 10)-(790, 70), style 0x30. */
s16 screen_heading(screen_stack_t *st, u16 colour, u16 text_colour,
                   const char *text);

/* FUN_00803f80. Font 6 in yellow, box (100, 80)-(790, 125), style 2. */
s16 screen_sub_heading(screen_stack_t *st, const char *text);

/* FUN_008040a0. A button in a cell of an area. The highlight is the colour. */
s16 screen_cell_button(screen_stack_t *st, s16 area, s32 column, s32 row,
                       u32 style, u16 colour, u32 text_flags, s16 font,
                       u16 text_colour, const char *text, s32 id,
                       intptr_t data);

/* FUN_00804100. A label in a cell of an area: no highlight, id or data. */
s16 screen_cell_label(screen_stack_t *st, s16 area, s32 column, s32 row,
                      u32 style, u16 colour, u32 text_flags, s16 font,
                      u16 text_colour, const char *text);

/* FUN_00804030. A button with its own box and no area. */
s16 screen_free_button(screen_stack_t *st, s32 x1, s32 y1, s32 x2, s32 y2,
                       u32 style, u16 colour, u32 text_flags, s16 font,
                       u16 text_colour, const char *text, s32 id,
                       intptr_t data);

#endif
