#include "guio.h"
#include "area.h"
#include "gui_font.h"
#include "gui_text.h"
#include "gui_utils.h"
#include "scrman.h"
#include "display.h"
#include <stdio.h>
#include <string.h>

extern cm_image_t *rgn_load(const char *path);
extern void rgn_free(cm_image_t *img);

/* One picture stays cached while widgets use it. Its file name shares it. */
static char g_cached_name[260];           /* DAT_00ae2920 */
static cm_image_t *g_cached_picture;                       /* DAT_00ae2a24 */
static s32 g_cached_users;                                 /* DAT_00ae291c */

static void load_picture(widget_t *widget)
{
    if (g_cached_picture && !strcmp(g_cached_name, widget->text)) {
        widget->picture = g_cached_picture;
        g_cached_users++;
        return;
    }

    widget->picture = rgn_load(widget->text);
    if (widget->picture && g_cached_users == 0) {
        rgn_free(g_cached_picture);
        g_cached_picture = widget->picture;
        strcpy(g_cached_name, widget->text);
        g_cached_users = 1;
    }
}

s32 g_tick;

/* FUN_005e5cd0, bitmap font part. */
static s32 font_height(s16 font)
{
    return font < 0 || font >= GUI_FONT_COUNT ? 0 : g_fonts[font].height;
}

/* The column after the one of the last widget in the area. */
static s32 next_column(area_t *area, const widget_t *widgets)
{
    s32 last = 0;

    /* The original divides by zero here. */
    if (area->column_count == 0) return 0;
    if (area->column_count == 1) return 0;
    area->unknown_b78 = 0;
    while (area->unknown_b78 < area->widget_count) {
        s16 index = area->widget_index[area->unknown_b78++];

        if (index == -1) break;
        last = widgets[index].column;
    }
    return (last + 1) % area->column_count;
}

void widget_init(widget_t *widget, struct area *areas, widget_t *widgets,
                 s16 index, u32 flags, s32 x1, s32 y1, s32 x2, s32 y2,
                 s32 column, s32 row, u32 style, u16 colour,
                 u16 highlight_colour, u32 text_flags, s16 font,
                 u16 text_colour, const char *text, s32 unknown_40,
                 s32 unknown_08, intptr_t unknown_48, s16 area)
{
    /* The original tests these two bits of the style, not of the flags. */
    bool keep_order = (style & 0x400) && !(style & 0x4000);
    bool in_area = area >= 0 && area < SCRMAN_MAX_AREAS;

    widget->areas = areas;
    widget->widgets = widgets;
    widget->lighten_once = 0;
    if (x2 < x1 && !keep_order) { s32 t = x1; x1 = x2; x2 = t; }
    if (y2 < y1 && !keep_order) { s32 t = y1; y1 = y2; y2 = t; }
    if (x1 < 1) x1 = 0;
    if (y1 < 1) y1 = 0;
    if (x2 > 799) x2 = 799;
    if (y2 > 599) y2 = 599;

    const char *text_given = text;

    if (!text) text = "";
    if (!in_area || areas[area].owner_widget == -1) {
        strncpy(widget->text, text, sizeof(widget->text) - 1);
        widget->text[sizeof(widget->text) - 1] = '\0';
    } else {
        area_t *popup = &areas[area];
        s32 width;

        snprintf(widget->text, sizeof(widget->text), "     %s", text);
        width = gui_line_width(font, widget->text) + 15;
        if (width < 126) width = 125;
        if (popup->x2 < width) popup->x2 = width;
        column = next_column(popup, widgets);
        row = popup->row_count;
        popup->y2 += font_height(font) + 6;
        if (row & 1) widget->lighten_once = 1;
        popup->row_count++;
    }

    widget->index = index;
    widget->flags = flags;
    widget->x1 = widget->fixed_x1 = x1;
    widget->y1 = widget->fixed_y1 = y1;
    widget->x2 = widget->fixed_x2 = x2;
    widget->y2 = widget->fixed_y2 = y2;
    widget->column = column;
    widget->row = row;
    widget->style = style;
    widget->colour = colour;
    widget->highlight_colour = highlight_colour;
    widget->text_flags = text_flags;
    widget->font = font;
    widget->text_colour = text_colour;
    widget->unknown_08 = unknown_08;
    widget->unknown_40 = unknown_40;
    widget->unknown_48 = unknown_48;
    widget->cursor = -1;
    widget->unknown_180 = 0;
    widget->saved = NULL;
    widget->area = area;
    widget->popup_area = -1;
    widget->highlighted = 0;
    widget->tick = g_tick;
    widget->unknown_7e = -1;
    widget->picture = NULL;
    widget->text_source = NULL;
    if (flags & WIDGET_TEXT_ENTRY) {
        widget->text_source = text_given;
        if (flags & WIDGET_CURSOR_AT_END) {
            widget->cursor = (s32)strlen(widget->text);
        }
    }
}

void widget_release(widget_t *widget)
{
    rgn_free(widget->saved);
    widget->saved = NULL;
    if (!widget->picture) return;
    if (strcmp(g_cached_name, widget->text)) {
        rgn_free(widget->picture);
    } else {
        /* The original takes the name as proof that the picture is shared. */
        g_cached_users--;
    }
    widget->picture = NULL;
}

static s32 centred(s32 low, s32 high, s32 size)
{
    return (high - size - low + 1) / 2 + low;
}

static u16 mark_colour(const widget_t *widget)
{
    return widget->colour == GUI_COLOUR_BLACK ? GUI_COLOUR_LIGHT_GREY
                                              : GUI_COLOUR_BLACK;
}

/* An arrow on the side where the popup area opens, or none if they overlap. */
static void draw_popup_arrow(const widget_t *widget, s32 dx, s32 dy)
{
    const area_t *popup = &widget->areas[widget->popup_area];
    const u8 *glyph;
    s32 popup_x = 0, popup_y = 0;
    s32 x;

    /* For a popup area with no owner, the original reads two unset values. */
    area_popup_position(popup, &popup_x, &popup_y);
    if (popup->y2 - popup->y1 + popup_y < widget->y1) {
        glyph = gui_glyph_up;
    } else if (widget->y2 < popup_y) {
        glyph = gui_glyph_down;
    } else if (popup->x2 - popup->x1 + popup_x < widget->x1) {
        glyph = gui_glyph_left;
    } else if (popup_x > widget->x2) {
        glyph = gui_glyph_right;
    } else {
        return;
    }

    if (widget->text[0] == '\0') {
        x = centred(widget->x1, widget->x2, glyph[0]);
    } else if (glyph == gui_glyph_left) {
        x = widget->x1 + 4;
    } else {
        x = widget->x2 - glyph[0] - 4;
    }
    gui_draw_glyph(x + dx, centred(widget->y1, widget->y2, glyph[1]) + dy,
                   mark_colour(widget), glyph);
}

/* With the T2K fonts, the original draws a mark as a text character. */
static void draw_mark(const widget_t *widget, s32 dx, s32 dy, const u8 *glyph,
                      u16 colour)
{
    gui_draw_glyph(widget->x2 - glyph[0] - 4 + dx,
                   centred(widget->y1, widget->y2, glyph[1]) + dy, colour,
                   glyph);
}

void widget_draw(widget_t *widget, bool redraw)
{
    u32 style = widget->style;
    u16 colour;
    s32 dx = 0, dy = 0;

    if (widget->lighten_once) {
        widget->lighten_once = 0;
        widget->colour = gui_scale_colour(widget->colour, 110);
    }
    if (widget->saved && redraw) {
        rgn_free(widget->saved);
        widget->saved = NULL;
    }
    if (!(style & GUI_PANEL_FILL) && !widget->saved) {
        widget->saved = gui_capture(widget->x1, widget->y1, widget->x2,
                                    widget->y2, NULL);
    }
    if ((widget->flags & WIDGET_PICTURE) && !widget->picture) {
        load_picture(widget);
    }
    gui_blit(widget->x1, widget->y1, widget->picture);

    colour = widget->colour;
    if (widget->highlighted) {
        colour = widget->highlight_colour;
        if ((style & GUI_PANEL_BORDER) &&
            !(widget->flags & WIDGET_STAYS_RAISED)) {
            style |= GUI_PANEL_SUNKEN;
        }
    }
    if (style & GUI_PANEL_SUNKEN) {
        dx = 2;
        dy = 2;
    }
    gui_panel(widget->x1, widget->y1, widget->x2, widget->y2, style, colour);

    if (widget->popup_area != -1 && !(widget->flags & WIDGET_NO_POPUP_ARROW)) {
        draw_popup_arrow(widget, dx, dy);
    }
    if (widget->flags & WIDGET_TICK) {
        gui_draw_glyph(widget->x1 + 4 + dx,
                       centred(widget->y1, widget->y2, gui_glyph_tick[1]) + dy,
                       mark_colour(widget), gui_glyph_tick);
    }
    if (widget->flags & WIDGET_MARK_RIGHT) {
        draw_mark(widget, dx, dy, gui_glyph_mark_right, GUI_COLOUR_ORANGE);
    }
    if (widget->flags & WIDGET_MARK_UP) {
        draw_mark(widget, dx, dy, gui_glyph_mark_up, GUI_COLOUR_ORANGE);
    }
    if (widget->flags & WIDGET_MARK_DOWN) {
        draw_mark(widget, dx, dy, gui_glyph_mark_down, GUI_COLOUR_RED);
    }

    if (!(widget->flags & WIDGET_PICTURE)) {
        char *stop = NULL;

        if (widget->flags & WIDGET_TEXT_STOP) {
            stop = strchr(widget->text, 1);
            if (stop) *stop = '\0';
        }
        gui_text(widget->x1 + dx, widget->y1 + dy, widget->x2 + dx,
                 widget->y2 + dy, widget->text_flags, widget->font,
                 widget->text_colour, widget->text, widget->cursor);
        if (stop) *stop = 1;
    }
}

void widget_set_highlight(widget_t *widget, bool on)
{
    widget->highlighted = on;
    widget->tick = g_tick;
    if ((widget->style & (GUI_PANEL_FILL | GUI_PANEL_BORDER)) &&
        !((widget->flags & WIDGET_STAYS_RAISED) &&
          widget->colour == widget->highlight_colour)) {
        if (widget->saved) gui_blit(widget->x1, widget->y1, widget->saved);
        widget_draw(widget, false);
        display_flip(widget->x1, widget->y1, widget->x2, widget->y2);
    }
    if (widget->area == -1) return;

    area_t *area = &widget->areas[widget->area];

    if (on) {
        area->unknown_210 = widget->index;
    } else if (area->unknown_210 == widget->index) {
        area->unknown_210 = -1;
    }
}
