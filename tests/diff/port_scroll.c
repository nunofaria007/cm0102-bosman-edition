/*
 * Commands of the port driver for the scrollbar input
 * (tests/diff/cases_scroll.py): the poll, the keys and the wheel, the
 * scroll of an area and the scroll position functions, on the screen manager
 * of the test scene.
 */
#include "port_scroll.h"
#include "port_scene.h"
#include "screen_stack.h"
#include "scrman_input.h"
#include <stdio.h>
#include <string.h>

/* The area whose scrollbar g_key_scrollbar is: -1 for none, -2 for another. */
static int key_scrollbar_area(void)
{
    scrman_t *sm = scene_manager();

    if (!g_key_scrollbar) return -1;
    for (int i = 0; i < SCRMAN_MAX_AREAS; i++) {
        if (g_key_scrollbar == &sm->areas[i].scrollbar) return i;
    }
    return -2;
}

bool scroll_command(const char *cmd, const char *args)
{
    scrman_t *sm = scene_manager();
    long long v[3] = {0, 0, 0};
    int count = sscanf(args, "%lld %lld %lld", &v[0], &v[1], &v[2]);

    if (!strcmp(cmd, "scroll_poll")) {
        s16 result = scrman_poll(sm);

        printf("%d %d %d %d\n", result, sm->mode, g_cursor_kind, g_left_held);
        return true;
    }
    if (!strcmp(cmd, "scroll_key")) {
        if (count != 2) return false;
        scrman_scroll_key((s32)v[0], (s32)v[1]);
        printf("%d %d %d %d\n", g_mouse_x, g_mouse_y, g_left_held,
               g_click_frames);
        return true;
    }
    if (!strcmp(cmd, "scroll_frames")) {
        if (count != 1) return false;
        g_click_frames = (s32)v[0];
        return true;
    }
    if (!strcmp(cmd, "scroll_count")) {
        scrman_count_click_frame();
        printf("%d %d\n", g_left_held, g_click_frames);
        return true;
    }
    if (!strcmp(cmd, "scroll_bar")) {
        printf("%d\n", key_scrollbar_area());
        return true;
    }
    if (!strcmp(cmd, "scroll_bar_set")) {
        if (count != 1 || v[0] >= SCRMAN_MAX_AREAS) return false;
        g_key_scrollbar = v[0] < 0 ? NULL : &sm->areas[v[0]].scrollbar;
        return true;
    }
    if (!strcmp(cmd, "scroll_stack")) {
        /* "scroll_stack player area row": the record of g_screen_stack. */
        if (count != 3) return false;
        g_screen_stack.record.player = (s16)v[0];
        g_screen_stack.record.unknown_22 = (s16)v[1];
        g_screen_stack.record.unknown_24 = (s32)v[2];
        g_screen_stack.record.screen_manager = sm;
        return true;
    }
    if (!strcmp(cmd, "scroll_srow")) {
        if (count != 1) return false;
        printf("%d\n", screen_stack_shown_row(&g_screen_stack, (s16)v[0]));
        return true;
    }
    if (strncmp(cmd, "scroll_", 7) != 0) return false;
    if (count < 1 || v[0] < 0 || v[0] >= SCRMAN_MAX_AREAS) return false;
    area_t *area = &sm->areas[v[0]];

    if (!strcmp(cmd, "scroll_area")) {
        if (count != 2) return false;
        area_scroll(area, (s32)v[1]);
        return true;
    }
    if (!strcmp(cmd, "scroll_row")) {
        printf("%d\n", area_shown_row(area));
        return true;
    }
    if (!strcmp(cmd, "scroll_to")) {
        if (count != 2) return false;
        area_set_scroll_to(area, (s32)v[1]);
        return true;
    }
    return false;
}
