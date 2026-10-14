#ifndef GUIO_H
#define GUIO_H

#include "cm_types.h"
#include <stdint.h>

struct area;

#define WIDGET_TEXT_ENTRY     0x00000008 /* the caller's text buffer is kept */
#define WIDGET_CURSOR_AT_END  0x00000010 /* with WIDGET_TEXT_ENTRY */
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
 * bytes with 1-byte packing. Each comment gives the original offset. A field
 * whose purpose is not known yet has its offset as its name.
 */
typedef struct widget {
    struct area *areas;    /* 0x00, the area array of the screen manager */
    struct widget *widgets; /* 0x04, the widget array of the screen manager */
    s32 unknown_08;
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
    s32 unknown_40;
    s32 cursor;            /* 0x44, place of the cursor mark, -1 = none */
    intptr_t unknown_48;
    cm_image_t *saved;     /* 0x4c, the screen under the widget */
    s32 tick;              /* 0x50, g_tick at the time of widget_init */
    const char *text_source; /* 0x54, the caller's text, for WIDGET_TEXT_ENTRY */
    cm_image_t *picture;   /* 0x5c */
    s16 index;             /* 0x70, of this widget */
    u16 colour;            /* 0x72 */
    u16 highlight_colour;  /* 0x74, panel colour while highlighted */
    s16 font;              /* 0x76, 0 to 7, -1 = no text */
    u16 text_colour;       /* 0x78 */
    s16 area;              /* 0x7a, index of the area it sits in, -1 = none */
    s16 popup_area;        /* 0x7c, index of the popup area it opens */
    s16 unknown_7e;        /* widget_init sets -1 */
    char text[256];        /* 0x80 */
    s32 unknown_180;
    s32 lighten_once;      /* 0x184, the next draw makes colour 10% lighter */
    s32 highlighted;       /* 0x188 */
} widget_t;

/* DAT_00b67a08. A tick count that the input code of the original sets. */
extern s32 g_tick;

/*
 * FUN_005ee150. Fills a zeroed widget. `area` is the index of the area it
 * sits in, or -1. A widget in a popup area ignores `column` and `row`: it
 * takes the next row of that area, gets five spaces before its text, and
 * makes the area wide enough for the text and one row higher.
 *
 * The original copies the text with no limit, and its 257th to 260th bytes
 * land in unknown_180, which it then clears. The port keeps 255 characters.
 * In a popup area with no columns the original divides by zero. The port
 * uses column 0. An area index outside the areas reads other memory in the
 * original. The port treats it as an area that is not a popup area.
 */
void widget_init(widget_t *widget, struct area *areas, widget_t *widgets,
                 s16 index, u32 flags, s32 x1, s32 y1, s32 x2, s32 y2,
                 s32 column, s32 row, u32 style, u16 colour,
                 u16 highlight_colour, u32 text_flags, s16 font,
                 u16 text_colour, const char *text, s32 unknown_40,
                 s32 unknown_08, intptr_t unknown_48, s16 area);

/*
 * FUN_005eef00. Frees the saved screen and the picture. A picture that the
 * widget shares stays loaded for the next widget that names its file.
 */
void widget_release(widget_t *widget);

/*
 * FUN_005eec20 and FUN_005eecb0. Turns the highlight of a button on or off,
 * notes the time, and draws and flips the widget again when its style has a
 * panel. The area of the widget records which widget it highlights.
 */
void widget_set_highlight(widget_t *widget, bool on);

/* FUN_005ee590. `redraw` drops the saved screen and captures it again. */
void widget_draw(widget_t *widget, bool redraw);

#endif
