#ifndef GUIO_H
#define GUIO_H

#include "cm_types.h"

struct area;

#define WIDGET_STAYS_RAISED   0x00000004 /* no sunken bevel when highlighted */
#define WIDGET_NO_POPUP_ARROW 0x00000080
#define WIDGET_PICTURE        0x00000400 /* the text is an RGN file name */
#define WIDGET_TEXT_STOP      0x00000800 /* the text ends at the first byte 1 */
#define WIDGET_TICK           0x00002000
#define WIDGET_MARK_RIGHT     0x00004000
#define WIDGET_MARK_UP        0x00020000
#define WIDGET_MARK_DOWN      0x00040000

/*
 * A widget, from Guio.cpp of the original. The original struct is 0x18c
 * bytes with 1-byte packing. Each comment gives the original offset. Fields
 * whose purpose is not known yet are left out.
 */
typedef struct widget {
    struct area *areas;    /* 0x00, the area array of the screen manager */
    u32 flags;             /* 0x0c */
    s32 x1, y1, x2, y2;    /* 0x10, on screen, set by area draw */
    s32 column;            /* 0x20, in the grid of its area */
    s32 row;               /* 0x24 */
    s32 fixed_x1;          /* 0x28, 0 = take both x values from the column */
    s32 fixed_x2;          /* 0x2c */
    s32 fixed_y1;          /* 0x30, 0 = take both y values from the row */
    s32 fixed_y2;          /* 0x34 */
    u32 style;             /* 0x38, panel flags */
    u32 text_flags;        /* 0x3c, passed to gui_text */
    s32 cursor;            /* 0x44, place of the cursor mark, -1 = none */
    cm_image_t *saved;     /* 0x4c, the screen under the widget */
    cm_image_t *picture;   /* 0x5c */
    u16 colour;            /* 0x72 */
    u16 highlight_colour;  /* 0x74, panel colour while highlighted */
    s16 font;              /* 0x76, 0 to 7, -1 = no text */
    u16 text_colour;       /* 0x78 */
    s16 area;              /* 0x7a, index of the area it sits in, -1 = none */
    s16 popup_area;        /* 0x7c, index of the popup area it opens */
    char text[260];        /* 0x80 */
    s32 lighten_once;      /* 0x184, the next draw makes colour 10% lighter */
    s32 highlighted;       /* 0x188 */
} widget_t;

/* FUN_005ee590. `redraw` drops the saved screen and captures it again. */
void widget_draw(widget_t *widget, bool redraw);

#endif
