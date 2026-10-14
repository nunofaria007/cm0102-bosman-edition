#include "scrman_input.h"
#include "display.h"
#include "gui_utils.h"
#include <stddef.h>

extern void rgn_free(cm_image_t *img);

s32 g_mouse_x;
s32 g_mouse_y;
s32 g_left_held;
s32 g_right_held;
s32 g_left_clicked;
s32 g_right_clicked;
u8 g_key_char;
s32 g_cursor_kind;
s32 g_click_frames;

#define WIDGET_BUTTON      0x00000002
#define WIDGET_RIGHT_CLICK 0x00000100 /* the right button presses it */
#define WIDGET_DRAG        0x00000040

/*
 * Entry i of the widget list of an area. An area with no widget in view has
 * first_visible -1, and the original then reads the field before the list.
 */
static s16 list_entry(const area_t *area, s16 i)
{
    return i == -1 ? area->unknown_210 : area->widget_index[i];
}

/* One widget of the hit test. Returns true when it is pressed. */
static bool test_widget(widget_t *w)
{
    s32 held = (w->flags & WIDGET_RIGHT_CLICK) ? g_right_held : g_left_held;
    bool inside = w->x1 <= g_mouse_x && g_mouse_x <= w->x2 &&
                  w->y1 <= g_mouse_y && g_mouse_y <= w->y2;

    if (!inside || !held) {
        if (w->highlighted) widget_set_highlight(w, false);
        if (inside && (w->unknown_08 != 0 || w->popup_area != -1)) {
            g_cursor_kind = 2;
        }
        return false;
    }
    if (!w->highlighted) widget_set_highlight(w, true);
    if (w->unknown_08 != 0 || w->popup_area != -1) g_cursor_kind = 2;
    return true;
}

/* Puts the screen under the scrollbar back, then draws and flips it again. */
static void redraw_scrollbar(area_t *area)
{
    area_scrollbar_t *bar = &area->scrollbar;

    if (bar->saved) {
        gui_blit(bar->x1, bar->y1, bar->saved);
        rgn_free(bar->saved);
        bar->saved = NULL;
    }
    area_draw_scrollbar(area);
    display_flip(bar->x1, bar->y1, bar->x2, bar->y2);
}

/*
 * FUN_00561f10. A click on the scrollbar of an area. The arrows move one row
 * and the track one page, once for each call while the button is held. A
 * click on the thumb starts the thumb drag (mode 2). Returns 1 when the
 * first row in view changed.
 */
static s32 scrollbar_click(scrman_t *sm, s16 index)
{
    area_t *area = &sm->areas[index];
    area_scrollbar_t *bar = &area->scrollbar;
    s32 old_position = bar->position;
    u8 old_pressed = bar->pressed;
    s32 x = g_mouse_x, y = g_mouse_y;
    bool across = bar->x1 <= x && x <= bar->x2;

    bar->pressed = 0;
    if (g_left_held && across) {
        if (bar->y1 <= y && y <= bar->track_y1 - 1) {
            bar->position = bar->position - 1 < 1 ? 0 : bar->position - 1;
            bar->pressed = 1;
        } else if (bar->track_y2 + 1 <= y && y <= bar->y2) {
            s32 position = bar->position + 1;

            bar->position = position < bar->max_position ? position
                                                         : bar->max_position;
            bar->pressed = 2;
        } else if (bar->track_y1 <= y && y <= bar->thumb_y1 - 1) {
            s32 position = bar->position - area->row_count;

            bar->position = position < 1 ? 0 : position;
        } else if (bar->thumb_y2 + 1 <= y && y <= bar->track_y2) {
            s32 position = bar->position + area->row_count;

            bar->position = position < bar->max_position ? position
                                                         : bar->max_position;
        } else if (bar->thumb_y1 <= y && y <= bar->thumb_y2) {
            sm->mode = 2;
            sm->unknown_12f52f = index;
            sm->unknown_12f509[1] = y;
        }
    }
    if (bar->position != old_position) {
        bar->thumb_y1 = (bar->track_y2 - bar->thumb_height - bar->track_y1 + 1) *
                        bar->position / bar->max_position + bar->track_y1;
        bar->thumb_y2 = bar->thumb_y1 - 1 + bar->thumb_height;
        redraw_scrollbar(area);
        area_scroll(area, bar->position - old_position);
        return 1;
    }
    if (bar->pressed != old_pressed) redraw_scrollbar(area);
    return 0;
}

/*
 * Walks the rows of an area that are in view, with unknown_b78 as the place
 * in the list. With `press`, tests each button and returns the last one
 * pressed, or -1.
 */
static s16 walk_rows(scrman_t *sm, area_t *area, bool press)
{
    s16 hit = -1;

    area->unknown_b78 = area->first_visible;
    while (area->unknown_b78 < area->widget_count) {
        s16 index = list_entry(area, area->unknown_b78++);

        if (index == -1) break;
        widget_t *w = &sm->widgets[index];
        const widget_t *first =
            &sm->widgets[list_entry(area, area->first_visible)];

        if (w->row - first->row > area->row_count - 1) break;
        if (press && (w->flags & WIDGET_BUTTON) && test_widget(w)) hit = index;
    }
    return hit;
}

s16 scrman_hit_test(scrman_t *sm)
{
    s16 hit = -1;

    /* The original runs two passes for text entry. Buttons count in the first. */
    for (s16 k = 0; k < sm->widget_count; k++) {
        s16 index = sm->widget_order[k];
        widget_t *w = &sm->widgets[index];

        if ((w->flags & WIDGET_BUTTON) && test_widget(w)) hit = index;
    }
    for (s16 k = 0; k < sm->area_count; k++) {
        area_t *area = &sm->areas[sm->area_order[k]];

        if (area->shown && area->owner_widget == -1) {
            s16 index = walk_rows(sm, area, true);

            if (index != -1) hit = index;
        }
        /* The scrollbar of an area out of view still takes clicks. */
        if (area->has_scrollbar) scrollbar_click(sm, sm->area_order[k]);
    }
    /* The second pass is for text entry. It walks the rows again. */
    for (s16 k = 0; k < sm->area_count; k++) {
        area_t *area = &sm->areas[sm->area_order[k]];

        if (area->shown && area->owner_widget == -1) walk_rows(sm, area, false);
    }
    if (hit == -1) return -1;

    widget_t *w = &sm->widgets[hit];

    if (w->flags & WIDGET_DRAG) {
        sm->mode = 3;
        sm->unknown_12f509[0] = g_mouse_x;
        sm->unknown_12f509[1] = g_mouse_y;
        sm->unknown_12f531 = hit;
        sm->unknown_12f509[2] = w->x1;
        sm->unknown_12f509[3] = w->y1;
        sm->unknown_12f509[4] = w->x2;
        sm->unknown_12f509[5] = w->y2;
        sm->unknown_12f521 = NULL;
        return -1;
    }
    return hit;
}

s16 scrman_poll(scrman_t *sm)
{
    s16 result;

    g_cursor_kind = 0;
    if (sm->unknown_12f4ff != -1 && sm->unknown_12f4fe != 0 &&
        g_key_char == sm->unknown_12f4fe) {
        g_key_char = 0;
        return sm->unknown_12f4ff;
    }
    if (g_left_clicked) {
        g_left_clicked = 0;
        if (sm->unknown_12f501 != -1) {
            result = sm->unknown_12f501;
            sm->unknown_12f501 = -1;
            return result;
        }
    }
    if (g_right_clicked) {
        g_right_clicked = 0;
        if (sm->unknown_12f503 != -1) {
            result = sm->unknown_12f503;
            sm->unknown_12f503 = -1;
            return result;
        }
    }
    if (sm->mode == 2) return scrman_scrollbar_drag(sm);
    if (sm->mode != 0) return -1;
    return scrman_hit_test(sm);
}

s16 scrman_scrollbar_drag(scrman_t *sm)
{
    s32 y = g_mouse_y;
    s32 dy = y - sm->unknown_12f509[1];

    if (!g_left_held) {
        sm->mode = 0;
        return -1;
    }
    if (dy == 0) return -1;

    area_t *area = &sm->areas[sm->unknown_12f52f];
    area_scrollbar_t *bar = &area->scrollbar;

    if (bar->thumb_y1 + dy < bar->track_y1) dy = bar->track_y1 - bar->thumb_y1;
    if (bar->track_y2 < bar->thumb_y2 + dy) dy = bar->track_y2 - bar->thumb_y2;
    if (dy != 0) {
        s32 old_position = bar->position;

        bar->thumb_y1 += dy;
        bar->thumb_y2 += dy;
        redraw_scrollbar(area);
        s32 room = bar->track_y2 - bar->thumb_height - bar->track_y1 + 1;

        /* The original divides by zero for a track no longer than the thumb. */
        bar->position = room > 0 ? (bar->thumb_y1 - bar->track_y1) *
                                       bar->max_position / room
                                 : 0;
        if (bar->position != old_position) {
            area_scroll(area, bar->position - old_position);
        }
    }
    sm->unknown_12f509[1] = y;
    return -1;
}

void scrman_scroll_key(s32 mode, s32 frames)
{
    const area_scrollbar_t *bar = g_key_scrollbar;

    if (!bar) return;
    g_left_held = 1;
    g_click_frames = frames;
    switch (mode) {
    case 0:
        g_mouse_x = bar->x1 + 1;
        g_mouse_y = bar->track_y2 + 2;
        break;
    case 1:
        g_mouse_x = bar->x1 + 1;
        g_mouse_y = bar->y1 + 1;
        break;
    case 2:
        g_mouse_x = bar->x1 + 1;
        g_mouse_y = bar->thumb_y2 + 2;
        break;
    case 3:
        g_mouse_x = bar->x1 + 1;
        g_mouse_y = bar->track_y1 + 1;
        break;
    default:
        g_left_held = 0;
        g_click_frames = 0;
        break;
    }
}

void scrman_count_click_frame(void)
{
    if (g_click_frames) {
        g_click_frames--;
        if (g_click_frames == 0) g_left_held = 0;
    }
}
