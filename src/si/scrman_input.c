#include "scrman_input.h"
#include "display.h"
#include "gui_font.h"
#include "gui_utils.h"
#include <stddef.h>
#include <stdio.h>
#include <string.h>

extern void rgn_free(cm_image_t *img);

s32 g_mouse_x;
s32 g_mouse_y;
s32 g_left_held;
s32 g_right_held;
s32 g_left_clicked;
s32 g_right_clicked;
s32 g_key_code;
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

/* Calls each text entry of the free widgets from place `from` in the order. */
static void sweep_widgets(scrman_t *sm, s16 from)
{
    for (s16 k = from; k < sm->widget_count; k++) {
        s16 index = sm->widget_order[k];

        if (sm->widgets[index].flags & WIDGET_TEXT_ENTRY) {
            scrman_text_entry(sm, index);
        }
    }
}

/*
 * Walks the rows of an area that are in view, with unknown_b78 as the place
 * in the list. With `hit`, tests each button in the first pass, puts the
 * last one pressed into *hit, and stops at a text entry that finishes: it
 * returns that widget, else -1. Without `hit`, only calls the text entries.
 */
static s16 walk_rows(scrman_t *sm, area_t *area, s16 pass, s16 *hit)
{
    area->unknown_b78 = area->first_visible;
    while (area->unknown_b78 < area->widget_count) {
        s16 index = list_entry(area, area->unknown_b78++);

        if (index == -1) break;
        widget_t *w = &sm->widgets[index];
        const widget_t *first =
            &sm->widgets[list_entry(area, area->first_visible)];

        if (w->row - first->row > area->row_count - 1) break;
        if (hit && pass == 1 && (w->flags & WIDGET_BUTTON) && test_widget(w)) {
            *hit = index;
        }
        if ((w->flags & WIDGET_TEXT_ENTRY) && scrman_text_entry(sm, index) &&
            hit) {
            return index;
        }
    }
    return -1;
}

/* An area that the hit test walks: in view, and not a popup area. */
static bool walked(const area_t *area)
{
    return area->shown && area->owner_widget == -1;
}

s16 scrman_hit_test(scrman_t *sm)
{
    s16 hit = -1;

    for (s16 pass = 1; pass <= 2; pass++) {
        for (s16 k = 0; k < sm->widget_count; k++) {
            s16 index = sm->widget_order[k];
            widget_t *w = &sm->widgets[index];

            if (pass == 1 && (w->flags & WIDGET_BUTTON) && test_widget(w)) {
                hit = index;
            }
            if ((w->flags & WIDGET_TEXT_ENTRY) && scrman_text_entry(sm, index)) {
                /* The finished widget itself gets the first of these calls. */
                sweep_widgets(sm, k);
                if (pass == 1) sweep_widgets(sm, 0);
                return index;
            }
        }
        for (s16 k = 0; k < sm->area_count; k++) {
            area_t *area = &sm->areas[sm->area_order[k]];

            if (walked(area)) {
                s16 done = walk_rows(sm, area, pass, &hit);

                if (done != -1) {
                    /*
                     * The original also has a sweep for the first pass after
                     * this one, but it starts after the last area, so it
                     * never runs.
                     */
                    for (s16 rest = k; rest < sm->area_count; rest++) {
                        area_t *other = &sm->areas[sm->area_order[rest]];

                        if (walked(other)) walk_rows(sm, other, pass, NULL);
                    }
                    return done;
                }
            }
            /* The scrollbar of an area out of view still takes clicks. */
            if (pass == 1 && area->has_scrollbar) {
                scrollbar_click(sm, sm->area_order[k]);
            }
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

/* Whether a text entry takes the character. The list keeps file names safe. */
static bool takes_char(const widget_t *w, u8 c)
{
    switch (c) {
    case '"': case '*': case '/': case '<': case '>': case '|':
    case '{': case '}': case '[': case ']': case '~':
        return false;
    case ':': case '\\':
        return (w->flags & WIDGET_TEXT_PATH) != 0;
    case '?':
        return (w->flags & WIDGET_TEXT_QUESTION) != 0;
    default:
        return c >= 0x20;
    }
}

/* Puts the screen under the widget back. */
static void restore_background(const widget_t *w)
{
    if (w->saved) gui_blit(w->x1, w->y1, w->saved);
}

/* The widget used the key and the character. */
static void clear_keys(void)
{
    g_key_code = 0;
    g_key_char = 0;
}

/* Starts the editing, with the cursor at the end of the text. */
static void start_editing(widget_t *w, const char *text)
{
    w->cursor = (s32)strlen(text);
    w->text_changed = 0;
    restore_background(w);
    widget_set_text(w, text, true);
}

bool scrman_text_entry(scrman_t *sm, s16 index)
{
    widget_t *w = &sm->widgets[index];
    char text[2 * sizeof(w->text)];
    s32 key = g_key_code;
    s32 cursor;
    s32 length;
    bool inside;

    if (!(w->flags & WIDGET_TEXT_ENTRY)) {
        g_left_held = 0;
        return false;
    }
    strcpy(text, w->text);
    inside = w->x1 <= g_mouse_x && g_mouse_x <= w->x2 &&
             w->y1 <= g_mouse_y && g_mouse_y <= w->y2;
    if (inside) g_cursor_kind = 2;

    /* A click inside starts the editing, a click outside ends it. */
    if (g_left_held && inside && w->cursor == -1) {
        start_editing(w, text);
        clear_keys();
        return false;
    }
    if (g_left_held && !inside && w->cursor > -1) {
        w->cursor = -1;
        restore_background(w);
        widget_set_text(w, text, true);
        clear_keys();
        return w->text_changed != 0;
    }

    /* Tab and Enter end it and pass the focus on. */
    if ((key == 0x09 || key == 0x0d) && w->cursor > -1) {
        w->cursor = -1;
        restore_background(w);
        widget_draw(w, false);
        display_flip(w->x1, w->y1, w->x2, w->y2);
        if (!(w->flags & WIDGET_KEEPS_FOCUS)) sm->passed_focus = index;
        clear_keys();
        widget_set_text(w, text, true);
        return w->text_changed != 0;
    }
    if (sm->passed_focus != -1 && index != sm->passed_focus &&
        w->cursor == -1) {
        start_editing(w, text);
        sm->passed_focus = -1;
        clear_keys();
        return false;
    }

    /* A negative limit: the first key empties the text and stays for the next call. */
    cursor = w->cursor;
    if (cursor > -1 && key != 0 && w->text_limit < 0) {
        widget_set_text(w, "", false);
        w->cursor = 0;
        w->text_changed = 1;
        w->text_limit = (s32)(0u - (u32)w->text_limit);
        restore_background(w);
        widget_draw(w, false);
        display_flip(w->x1, w->y1, w->x2, w->y2);
        return false;
    }

    length = (s32)strlen(text);
    if (cursor > 0 && key == 0x25) {                       /* Left */
        w->cursor = cursor - 1;
    } else if (cursor > -1 && cursor < length && key == 0x27) { /* Right */
        w->cursor = cursor + 1;
    } else if (cursor > 0 && key == 0x08) {                /* Backspace */
        if (cursor <= length) {
            memmove(text + cursor - 1, text + cursor, (size_t)(length - cursor + 1));
        }
        w->cursor = cursor - 1;
        w->text_changed = 1;
    } else if (cursor > -1 && cursor < length && key == 0x2e) { /* Delete */
        memmove(text + cursor, text + cursor + 1, (size_t)(length - cursor));
        w->text_changed = 1;
    } else {
        char probe[sizeof(text) + 2];
        u8 c = g_key_char;

        if (cursor <= -1 || length >= w->text_limit) return false;
        if (!takes_char(w, c)) return false;
        snprintf(probe, sizeof(probe), "%s%c", text, c);
        if (gui_line_width(w->font, probe) > w->x2 - w->x1 - 10) {
            clear_keys();
            return false;
        }
        /* The port keeps 255 characters, the size of the widget text. */
        if (length >= (s32)sizeof(w->text) - 1 ||
            cursor >= (s32)sizeof(w->text)) {
            clear_keys();
            return false;
        }
        if (cursor < length) {
            memmove(text + cursor + 1, text + cursor, (size_t)(length - cursor));
        }
        text[cursor] = (char)c;
        text[length + 1] = '\0';
        w->cursor = cursor + 1;
        w->text_changed = 1;
    }
    restore_background(w);
    widget_set_text(w, text, true);
    clear_keys();
    return false;
}
