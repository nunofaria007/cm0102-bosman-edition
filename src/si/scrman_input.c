#include "scrman_input.h"
#include <stddef.h>

s32 g_mouse_x;
s32 g_mouse_y;
s32 g_left_held;
s32 g_right_held;
s32 g_left_clicked;
s32 g_right_clicked;
u8 g_key_char;
s32 g_cursor_kind;

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

        if (!area->shown || area->owner_widget != -1) continue;
        area->unknown_b78 = area->first_visible;
        while (area->unknown_b78 < area->widget_count) {
            s16 index = list_entry(area, area->unknown_b78++);

            if (index == -1) break;
            widget_t *w = &sm->widgets[index];
            const widget_t *first =
                &sm->widgets[list_entry(area, area->first_visible)];

            if (w->row - first->row > area->row_count - 1) break;
            if ((w->flags & WIDGET_BUTTON) && test_widget(w)) hit = index;
        }
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
    if (sm->mode != 0) return -1;
    return scrman_hit_test(sm);
}
