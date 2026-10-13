#include "guio.h"
#include "area.h"
#include "gui_text.h"
#include "gui_utils.h"
#include <string.h>

extern cm_image_t *rgn_load(const char *path);
extern void rgn_free(cm_image_t *img);

s32 guio_symbol_font = -2;

/* The last picture loaded. Widgets that name the same file share it. */
static char cached_name[sizeof(((widget_t *)0)->text)]; /* DAT_00ae2920 */
static cm_image_t *cached_picture;                       /* DAT_00ae2a24 */
static s32 cached_users;                                 /* DAT_00ae291c */

static void load_picture(widget_t *widget)
{
    if (cached_picture && !strcmp(cached_name, widget->text)) {
        widget->picture = cached_picture;
        cached_users++;
        return;
    }

    widget->picture = rgn_load(widget->text);
    if (widget->picture && cached_users == 0) {
        rgn_free(cached_picture);
        cached_picture = widget->picture;
        strcpy(cached_name, widget->text);
        cached_users = 1;
    }
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

static void draw_mark(const widget_t *widget, s32 dx, s32 dy, const u8 *glyph,
                      u16 colour, char symbol)
{
    if (guio_symbol_font == -1) {
        gui_draw_glyph(widget->x2 - glyph[0] - 4 + dx,
                       centred(widget->y1, widget->y2, glyph[1]) + dy, colour,
                       glyph);
    } else {
        const char text[2] = { symbol, '\0' };

        gui_text(widget->x1 + dx, widget->y1 + dy, widget->x2 - 4 + dx,
                 widget->y2 + dy, 0x48, 0, widget->second_colour, text, -1);
    }
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
        draw_mark(widget, dx, dy, gui_glyph_mark_right, GUI_COLOUR_ORANGE, '+');
    }
    if (widget->flags & WIDGET_MARK_UP) {
        draw_mark(widget, dx, dy, gui_glyph_mark_up, GUI_COLOUR_ORANGE, ',');
    }
    if (widget->flags & WIDGET_MARK_DOWN) {
        draw_mark(widget, dx, dy, gui_glyph_mark_down, GUI_COLOUR_RED, '-');
    }

    if (!(widget->flags & WIDGET_PICTURE)) {
        char *stop = NULL;

        if (widget->flags & WIDGET_TEXT_STOP) {
            stop = strchr(widget->text, 1);
            if (stop) *stop = '\0';
        }
        gui_text(widget->x1 + dx, widget->y1 + dy, widget->x2 + dx,
                 widget->y2 + dy, widget->text_flags, widget->text_colour,
                 widget->second_colour, widget->text, widget->text_param);
        if (stop) *stop = 1;
    }
}
