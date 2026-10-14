#ifndef SCRMAN_H
#define SCRMAN_H

#include "area.h"

#define SCRMAN_MAX_AREAS   250
#define SCRMAN_MAX_WIDGETS 1200

/*
 * The screen manager, from display.cpp of the original. Each comment gives
 * the original offset. A field whose purpose is not known yet has its offset
 * as its name.
 */
typedef struct {
    s32 mode;                               /* 0x000000, of the input code */
    area_t areas[SCRMAN_MAX_AREAS];         /* 0x000004 */
    widget_t widgets[SCRMAN_MAX_WIDGETS];   /* 0x0ba95e */
    s16 area_slots;                         /* 0x12e99e, areas in use */
    s16 widget_slots;                       /* 0x12e9a0, widgets in use */
    s16 area_order[SCRMAN_MAX_AREAS];       /* 0x12e9a2, draw order */
    s16 widget_order[SCRMAN_MAX_WIDGETS];   /* 0x12eb96, widgets with no area */
    s16 area_count;                         /* 0x12f4f6, in area_order */
    s16 widget_count;                       /* 0x12f4f8, in widget_order */
    s32 clear_screen;                       /* 0x12f4fa */
    u8 unknown_12f4fe;
    s16 unknown_12f4ff;
    s16 unknown_12f501;
    s16 unknown_12f503;
    s32 unknown_12f505;
    s32 unknown_12f509[6];
    cm_image_t *unknown_12f521;
    cm_image_t *background;                 /* 0x12f525 */
    void *unknown_12f529;
    s16 unknown_12f52d;
    s16 unknown_12f52f;
    s16 unknown_12f531;
    s16 unknown_12f533;
    s32 darken_once;                        /* 0x12f535 */
    s16 unknown_12f539;
    s16 unknown_12f53b;
    s32 skip_draw;                          /* 0x12f53d */
    u32 unknown_12f541;                     /* the argument of scrman_init */
    s32 laid_out;                           /* 0x12f545, set by the first draw */
    s32 widget_overflow;                    /* 0x12f549, a create found no slot */
    s32 area_overflow;                      /* 0x12f54d */
    s32 unknown_12f551;
    s32 unknown_12f555;                     /* not 0: the reset releases nothing */
    char background_name[260];              /* 0x12f55d */
} scrman_t;

/*
 * FUN_0055ef50. Leaves area_count and widget_count as they are, as the
 * original does, so call scrman_reset before the first create.
 */
void scrman_init(scrman_t *sm, u32 arg);

/*
 * FUN_0055f7c0. Empties the screen manager and frees unknown_12f521.
 * `release` releases every area and widget in use first (area_release,
 * widget_release), unless unknown_12f555 is set. Without that, the next
 * create that takes the slot of an area or widget drops its saved screens
 * and its picture without freeing them, as the original does.
 */
void scrman_reset(scrman_t *sm, bool release);

/*
 * FUN_0055fba0. Returns the index of the new area, or -1 when 249 areas are
 * in use: the original never uses the last of its 250 slots. A weight table
 * holds one byte for each column or row. The original ignores it when it is
 * NULL or the count is 0 or 30. A count above 30 writes past the tables in
 * the original. The port sets 30 weights then.
 */
s16 scrman_area_create(scrman_t *sm, s32 x1, s32 y1, s32 x2, s32 y2,
                       u8 column_count, const u8 *column_weights,
                       u8 row_count, const u8 *row_weights,
                       u32 flags, u16 colour, s16 owner_widget);

/*
 * FUN_0055f990. Returns the index of the new widget, or -1 when 1199 widgets
 * are in use: the original never uses the last of its 1200 slots. A widget
 * with an area goes into that area (area_add_widget). A widget with area -1
 * goes into the draw order of the screen manager. See widget_init for the
 * rest.
 */
s16 scrman_widget_create(scrman_t *sm, u32 flags, s32 x1, s32 y1, s32 x2,
                         s32 y2, s32 column, s32 row, u32 style, u16 colour,
                         u16 highlight_colour, u32 text_flags, s16 font,
                         u16 text_colour, const char *text, s32 unknown_40,
                         s32 unknown_08, intptr_t unknown_48, s16 area);

#endif
