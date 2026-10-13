#ifndef SCRMAN_H
#define SCRMAN_H

#include "area.h"

#define SCRMAN_MAX_AREAS   250
#define SCRMAN_MAX_WIDGETS 1200

/*
 * The screen manager, from scrman.cpp of the original. Each comment gives
 * the original offset. Only the fields that FUN_0055f300 (screen draw)
 * proves are listed.
 */
typedef struct {
    area_t areas[SCRMAN_MAX_AREAS];         /* 0x000004 */
    widget_t widgets[SCRMAN_MAX_WIDGETS];   /* 0x0ba95e */
    s16 area_order[SCRMAN_MAX_AREAS];       /* 0x12e9a2, draw order */
    s16 widget_order[SCRMAN_MAX_WIDGETS];   /* 0x12eb96, widgets with no area */
    s16 area_count;                         /* 0x12f4f6 */
    s16 widget_count;                       /* 0x12f4f8, in widget_order */
    cm_image_t *background;                 /* 0x12f525 */
    s32 laid_out;                           /* 0x12f545, set by the first draw */
} scrman_t;

#endif
