#ifndef GUIO_H
#define GUIO_H

#include "cm_types.h"

struct area;

/*
 * A widget, from Guio.cpp of the original. The original struct is 0x18c
 * bytes with 1-byte packing. Each comment gives the original offset. Fields
 * whose purpose is not known yet are left out.
 */
typedef struct widget {
    struct area *areas;   /* 0x00, the area array of the screen manager */
    u32 flags;            /* 0x0c */
    s32 x1, y1, x2, y2;   /* 0x10, on screen, set by area draw */
    s32 column;           /* 0x20, in the grid of its area */
    s32 row;              /* 0x24 */
    s32 fixed_x1;         /* 0x28, 0 = take both x values from the column */
    s32 fixed_x2;         /* 0x2c */
    s32 fixed_y1;         /* 0x30, 0 = take both y values from the row */
    s32 fixed_y2;         /* 0x34 */
    u32 style;            /* 0x38, panel flags */
    cm_image_t *saved;    /* 0x4c, the screen under the widget */
    cm_image_t *picture;  /* 0x5c */
    u16 colour;           /* 0x72 */
    s16 area;             /* 0x7a, index of the area it sits in, -1 = none */
    s16 popup_area;       /* 0x7c, index of the popup area it opens */
    char text[260];       /* 0x80 */
} widget_t;

#endif
