#ifndef AREA_H
#define AREA_H

#include "cm_types.h"
#include "guio.h"

#define AREA_MAX_COLUMNS 30
#define AREA_MAX_ROWS    30
#define AREA_MAX_WIDGETS 1200

/* Area flags. The same word goes to gui_panel, so GUI_PANEL_ flags apply too. */
#define AREA_NO_MARGIN          0x00000001
#define AREA_POPUP_BELOW        0x00000100 /* opens under its widget */
#define AREA_GRID_ONLY          0x00080000 /* grid lines, no widgets */
#define AREA_POPUP_LEFT         0x00100000
#define AREA_POPUP_RIGHT        0x00200000
#define AREA_POPUP_BOTTOM_ALIGN 0x00400000
#define AREA_POPUP_TOP_ALIGN    0x00800000

typedef struct {
    s32 x1, y1, x2, y2;       /* 0xb7a */
    s32 track_y1, track_y2;   /* 0xb8a, between the two arrow buttons */
    s32 max_position;         /* 0xb92, rows that do not fit */
    s32 position;             /* 0xb96, first row shown */
    s32 thumb_height;         /* 0xb9a */
    s32 thumb_y1, thumb_y2;   /* 0xb9e */
    cm_image_t *saved;        /* 0xba6, the screen under the scrollbar */
    u8 pressed;               /* 0xbaa, 1 = up arrow, 2 = down arrow */
} area_scrollbar_t;

/*
 * An area, from Area.cpp of the original. The original struct is 0xbf1
 * bytes with 1-byte packing. Each comment gives the original offset.
 */
typedef struct area {
    s32 x1, y1, x2, y2;                   /* 0x000, on screen */
    s32 row_offset;                       /* 0x010, added to a shown row */
    s32 last_row;                         /* 0x014, highest row of a widget */
    u32 flags;                            /* 0x018 */
    s32 column_x1[AREA_MAX_COLUMNS];      /* 0x01c, relative to x1 */
    s32 row_y1[AREA_MAX_ROWS];            /* 0x094, relative to y1 */
    s32 column_x2[AREA_MAX_COLUMNS];      /* 0x10c */
    s32 row_y2[AREA_MAX_ROWS];            /* 0x184 */
    s32 scroll_to;                        /* 0x1fc, row to show, -1 = none */
    cm_image_t *saved;                    /* 0x200, the screen under it */
    struct area *areas;                   /* 0x204, of the screen manager */
    widget_t *widgets;                    /* 0x208, of the screen manager */
    u16 colour;                           /* 0x20c */
    s16 owner_widget;                     /* 0x20e, set for a popup area */
    s16 widget_index[AREA_MAX_WIDGETS];   /* 0x212, its widgets in row order */
    s16 index;                            /* 0xb72, of this area */
    s16 widget_count;                     /* 0xb74 */
    s16 first_visible;                    /* 0xb76, into widget_index */
    area_scrollbar_t scrollbar;           /* 0xb7a */
    u8 column_count;                      /* 0xbab */
    u8 row_count;                         /* 0xbac, rows shown at once */
    u8 column_weight[AREA_MAX_COLUMNS];   /* 0xbad */
    u8 row_weight[AREA_MAX_ROWS];         /* 0xbcb */
    s32 shown;                            /* 0xbe9, screen draw skips it at 0 */
    s32 has_scrollbar;                    /* 0xbed */
} area_t;

/*
 * FUN_00403350. Splits the area into columns and rows by weight and sets up
 * the scrollbar when there are more rows than fit. The weights of two or
 * more columns or rows must not all be 0: the original divides by their sum.
 */
void area_layout(area_t *area);

/* FUN_00403760. Top-left corner at which a popup area opens. */
void area_popup_position(const area_t *area, s32 *x, s32 *y);

#endif
