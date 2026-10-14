#include "area.h"
#include "display.h"
#include "gui_utils.h"
#include <stddef.h>
#include <string.h>

extern void rgn_free(cm_image_t *img);

#define BUTTON_RAISED  (GUI_PANEL_FILL | GUI_PANEL_BORDER)
#define BUTTON_PRESSED (BUTTON_RAISED | GUI_PANEL_SUNKEN)

static void set_weights(u8 *weights, u8 count, const u8 *table)
{
    /* The original accepts a table for 1 to 29 columns or rows only. */
    if (table && count != 0 && count < AREA_MAX_COLUMNS) {
        memcpy(weights, table, count);
    } else {
        memset(weights, 1, count > AREA_MAX_COLUMNS ? AREA_MAX_COLUMNS : count);
    }
}

void area_init(area_t *area, struct area *areas, widget_t *widgets, s16 index,
               s32 x1, s32 y1, s32 x2, s32 y2,
               u8 column_count, const u8 *column_weights,
               u8 row_count, const u8 *row_weights,
               u32 flags, u16 colour, s16 owner_widget)
{
    if (owner_widget == -1) {
        if (x2 < x1) { s32 t = x1; x1 = x2; x2 = t; }
        if (y2 < y1) { s32 t = y1; y1 = y2; y2 = t; }
    } else {
        x1 = y1 = x2 = y2 = 0;
        memset(area->column_weight, 1, sizeof(area->column_weight));
        memset(area->row_weight, 1, sizeof(area->row_weight));
        column_count = 1;
        row_count = 0;
        column_weights = NULL;
        row_weights = NULL;
    }
    if (x2 > 799) x2 = 799;
    if (y2 > 599) y2 = 599;
    area->x1 = x1 < 1 ? 0 : x1;
    area->y1 = y1 < 1 ? 0 : y1;
    area->x2 = x2;
    area->y2 = y2;
    area->row_count = row_count;
    area->column_count = column_count;
    area->first_row = 0x7fffffff;
    area->last_row = 0;
    memset(&area->scrollbar, 0, sizeof(area->scrollbar));
    area->owner_widget = owner_widget;
    area->areas = areas;
    area->widgets = widgets;
    area->index = index;
    area->colour = colour;
    area->has_scrollbar = 0;
    area->saved = NULL;
    area->unknown_210 = -1;
    area->shown = 1;
    area->first_visible = -1;
    area->flags = flags;
    area->scroll_to = -1;
    if (flags & AREA_GRID_ONLY) return;
    set_weights(area->column_weight, column_count, column_weights);
    set_weights(area->row_weight, row_count, row_weights);
}

void area_add_widget(area_t *area, s16 widget)
{
    s16 count = area->widget_count;

    if (widget == -1 || count >= AREA_MAX_WIDGETS) return;
    if (area->flags & AREA_GRID_ONLY) return;

    s32 row = area->widgets[widget].row;

    area->widget_count = count + 1;
    if (row < area->first_row) area->first_row = row;
    if (row > area->last_row) area->last_row = row;
    while (--count >= 0) {
        s16 other = area->widget_index[count];

        if (area->widgets[other].row <= row) break;
        area->widget_index[count + 1] = other;
    }
    area->widget_index[count + 1] = widget;
}

area_scrollbar_t *g_key_scrollbar;

/* FUN_00403600 */
static void setup_scrollbar(area_t *area, s32 margin_x, s32 margin_y)
{
    area_scrollbar_t *bar = &area->scrollbar;
    s32 rows = area->row_count;

    bar->x2 = area->x2 - margin_x;
    bar->x1 = bar->x2 - 19;
    bar->y1 = area->y1 + margin_y;
    bar->y2 = area->y2 - margin_y;
    bar->track_y1 = bar->y1 + 20;
    bar->track_y2 = bar->y2 - 20;

    if (area->scroll_to == -1) {
        bar->position = 0;
    } else {
        s16 first = 0;

        if (area->scroll_to < 0) area->scroll_to = 0;
        if (area->scroll_to > area->last_row) area->scroll_to = area->last_row;
        while (first < area->widget_count &&
               area->widgets[area->widget_index[first]].row < area->scroll_to) {
            first++;
        }
        area->first_visible = first;
        bar->position = area->scroll_to;
    }

    s32 track = bar->track_y2 - bar->track_y1 + 1;

    bar->max_position = area->last_row - rows + 1;
    bar->thumb_height = track * rows / (bar->max_position + rows);
    if (bar->thumb_height < 11) bar->thumb_height = 10;
    bar->saved = NULL;
    bar->pressed = 0;
    area->has_scrollbar = 1;
    bar->thumb_y1 = (track - bar->thumb_height) * bar->position /
                    bar->max_position + bar->track_y1;
    bar->thumb_y2 = bar->thumb_y1 - 1 + bar->thumb_height;
    g_key_scrollbar = bar;
}

static void split(s32 *start, s32 *end, const u8 *weight, s32 count,
                  s32 margin, s32 size)
{
    s32 total = 0, sum = 0;

    for (s32 i = 0; i < count; i++) {
        total += weight[i];
    }
    if (total == 0) total = 1; /* the original faults */
    for (s32 i = 0; i < count; i++) {
        sum += weight[i];
        start[i] = i == 0 ? margin : end[i - 1] + 2;
        end[i] = (i < count - 1 ? sum * size / total : size) - 1 + margin;
    }
}

void area_layout(area_t *area)
{
    s32 columns = area->column_count;
    s32 rows = area->row_count;
    s32 margin_x = 2, margin_y = 8;

    if (area->owner_widget != -1) {
        area->shown = 0;
        area->widgets[area->owner_widget].popup_area = area->index;
    }
    area->first_visible = 0;

    if ((area->flags & AREA_GRID_ONLY) || columns == 0) return;
    /* The original shows an error box for more than 30 (Area.cpp 497, 507). */
    if (columns > AREA_MAX_COLUMNS || rows == 0 || rows > AREA_MAX_ROWS) return;

    if (area->flags & AREA_NO_MARGIN) {
        margin_x = 0;
        margin_y = 0;
    } else if ((area->flags & GUI_PANEL_FILL) || rows == 1) {
        margin_y = 2;
    }

    s32 width = area->x2 - area->x1 + 1 - 2 * margin_x;
    s32 height = area->y2 - area->y1 + 1 - 2 * margin_y;

    if (rows < area->last_row + 1) {
        setup_scrollbar(area, margin_x, margin_y);
        width -= 22;
    }
    split(area->column_x1, area->column_x2, area->column_weight, columns,
          margin_x, width);
    split(area->row_y1, area->row_y2, area->row_weight, rows, margin_y,
          height);
}

void area_draw_scrollbar(area_t *area)
{
    area_scrollbar_t *bar = &area->scrollbar;
    s32 up = bar->pressed == 1 ? 2 : 0;
    s32 down = bar->pressed == 2 ? 2 : 0;
    s32 glyph_x = bar->x1 + (20 - gui_glyph_up[0]) / 2;
    s32 glyph_y = (20 - gui_glyph_up[1]) / 2;

    if (!bar->saved) {
        bar->saved = gui_capture(bar->x1, bar->y1, bar->x2, bar->y2, NULL);
    }

    gui_panel(bar->x1, bar->y1, bar->x2, bar->track_y1 - 1,
              up ? BUTTON_PRESSED : BUTTON_RAISED, GUI_COLOUR_GREY);
    gui_draw_glyph(glyph_x + up, bar->y1 + glyph_y + up, GUI_COLOUR_BLACK,
                   gui_glyph_up);
    gui_panel(bar->x1, bar->track_y1, bar->x2, bar->track_y2, GUI_PANEL_DARKEN,
              GUI_COLOUR_BLACK);
    gui_panel(bar->x1, bar->thumb_y1, bar->x2, bar->thumb_y2, BUTTON_RAISED,
              GUI_COLOUR_GREY);
    gui_panel(bar->x1, bar->track_y2 + 1, bar->x2, bar->y2,
              down ? BUTTON_PRESSED : BUTTON_RAISED, GUI_COLOUR_GREY);
    gui_draw_glyph(glyph_x + down, bar->track_y2 + 1 + glyph_y + down,
                   GUI_COLOUR_BLACK, gui_glyph_down);
}

static void draw_grid(const area_t *area)
{
    s32 width = area->x2 - area->x1 + 1;
    s32 height = area->y2 - area->y1 + 1;

    for (s32 i = 1; i < area->row_count; i++) {
        s32 y = height * i / area->row_count + area->y1;

        gui_line(area->x1, y, area->x2, y, GUI_LINE_SOLID, GUI_COLOUR_GREY);
    }
    for (s32 i = 1; i < area->column_count; i++) {
        s32 x = width * i / area->column_count + area->x1;

        gui_line(x, area->y1, x, area->y2, GUI_LINE_SOLID, GUI_COLOUR_GREY);
    }
}

static void place_widget(const area_t *area, widget_t *widget, s32 shown_row)
{
    s32 row = area->first_row + shown_row;

    widget->x1 = widget->fixed_x1;
    widget->x2 = widget->fixed_x2;
    widget->y1 = widget->fixed_y1;
    widget->y2 = widget->fixed_y2;
    if (widget->fixed_x1 == 0) {
        widget->x1 = area->column_x1[widget->column] + area->x1;
        widget->x2 = area->column_x2[widget->column] + area->x1;
    }
    if (widget->fixed_y1 == 0) {
        widget->y1 = area->row_y1[row] + area->y1;
        widget->y2 = area->row_y2[row] + area->y1;
    }
}

void area_draw(area_t *area, bool redraw)
{
    /* The original shows an error box first (Area.cpp 245). */
    if (area->first_visible == -1) area_layout(area);

    if (redraw && area->saved) {
        rgn_free(area->saved);
        area->saved = NULL;
    }
    if (area->owner_widget != -1 && !area->saved) {
        area->saved = gui_capture(area->x1, area->y1, area->x2, area->y2, NULL);
    }
    gui_panel(area->x1, area->y1, area->x2, area->y2, area->flags,
              area->colour);

    if (area->flags & AREA_GRID_ONLY) {
        draw_grid(area);
        return;
    }
    if (area->has_scrollbar && !area->saved) {
        area->saved = gui_capture(area->x1, area->y1, area->x2 - 22, area->y2,
                                  NULL);
    }

    for (s16 i = area->first_visible; i < area->widget_count; i++) {
        widget_t *first = &area->widgets[area->widget_index[area->first_visible]];
        widget_t *widget = &area->widgets[area->widget_index[i]];
        s32 shown_row = widget->row - first->row;

        if (shown_row > area->row_count - 1) break;
        place_widget(area, widget, shown_row);
        widget_draw(widget, redraw);
    }
    if (area->has_scrollbar) area_draw_scrollbar(area);
}

void area_release(area_t *area)
{
    rgn_free(area->scrollbar.saved);
    area->scrollbar.saved = NULL;
    rgn_free(area->saved);
    area->saved = NULL;
    if (g_key_scrollbar == &area->scrollbar) g_key_scrollbar = NULL;
}

/* The row of the widget in list place i. Place -1 reads the field before the list. */
static s32 list_row(const area_t *area, s16 i)
{
    return area->widgets[i == -1 ? area->unknown_210 : area->widget_index[i]].row;
}

void area_scroll(area_t *area, s32 rows)
{
    s16 first = area->first_visible;
    s32 moved = 0;

    if (area->row_count == 0 || area->widget_count <= 0) return;
    if (area->saved) gui_blit(area->x1, area->y1, area->saved);

    if (rows < 1) {
        while (rows < moved && first - 1 >= 0) {
            first--;
            moved = list_row(area, first) - list_row(area, area->first_visible);
        }
        /* Back to the first widget of that row. */
        while (first != 0 && list_row(area, first - 1) == list_row(area, first)) {
            first--;
        }
    } else {
        while (moved < rows && first + 1 < area->widget_count) {
            first++;
            moved = list_row(area, first) - list_row(area, area->first_visible);
        }
    }
    area->first_visible = first;

    for (s16 i = first; i != -1 && i < area->widget_count; i++) {
        widget_t *widget = &area->widgets[area->widget_index[i]];
        s32 shown_row = widget->row - list_row(area, area->first_visible);

        if (shown_row > area->row_count - 1) break;
        place_widget(area, widget, shown_row);
        widget_draw(widget, false);
    }
    display_flip(area->x1, area->y1, area->x2 - 22, area->y2);
}

s32 area_shown_row(const area_t *area)
{
    if (!area->has_scrollbar) return 0;
    return list_row(area, area->first_visible);
}

void area_set_scroll_to(area_t *area, s32 row)
{
    s32 last = area->last_row - area->row_count + 1;

    if (row >= last) row = last;
    area->scroll_to = row < 1 ? 0 : row;
}

void area_popup_position(const area_t *area, s32 *x, s32 *y)
{
    /* The original shows an error box here (Area.cpp 718). */
    if (area->owner_widget == -1) return;

    const widget_t *owner = &area->widgets[area->owner_widget];
    const area_t *parent = owner->area != -1 ? &area->areas[owner->area] : NULL;
    s32 width = area->x2 - area->x1;
    s32 height = area->y2 - area->y1;
    bool nested = parent && parent->owner_widget != -1 &&
                  !(parent->flags & AREA_POPUP_BELOW);
    bool fits_down = owner->y1 + height < CM_INTERNAL_HEIGHT;
    bool top_aligned, to_right;

    if (!nested && (area->flags & AREA_POPUP_BELOW)) {
        *y = owner->y2 + 3 + height < CM_INTERNAL_HEIGHT ?
             owner->y2 + 3 : owner->y1 - height - 3;
        *x = owner->x1 <= CM_INTERNAL_WIDTH / 2 ? owner->x1 : owner->x2 - width;
        return;
    }

    if (nested) {
        top_aligned = fits_down;
        to_right = area->widgets[parent->owner_widget].x2 < parent->x1;
    } else {
        top_aligned = (fits_down || (area->flags & AREA_POPUP_TOP_ALIGN)) &&
                      !(area->flags & AREA_POPUP_BOTTOM_ALIGN);
        to_right = (owner->x1 <= CM_INTERNAL_WIDTH / 2 ||
                    (area->flags & AREA_POPUP_RIGHT)) &&
                   !(area->flags & AREA_POPUP_LEFT);
    }

    if (top_aligned) {
        *y = owner->y1;
    } else {
        *y = owner->y2 - height < 1 ? 0 : owner->y2 - height;
    }
    *x = to_right ? owner->x2 + 3 : owner->x1 - width - 3;
}
