#include "area.h"
#include "gui_utils.h"
#include <stddef.h>

/* FUN_00403600. The original also stores the scrollbar address in DAT_00ad9c58. */
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
}

static void split(s32 *start, s32 *end, const u8 *weight, s32 count,
                  s32 margin, s32 size)
{
    s32 total = 0, sum = 0;

    for (s32 i = 0; i < count; i++) total += weight[i];
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

    /* The original shows an error box for more than 30 (Area.cpp 497, 507). */
    if ((area->flags & AREA_GRID_ONLY) || columns == 0) return;
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
