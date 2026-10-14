"""Deliberate breaks of the scrollbar input. Each one must fail a suite. See tools/breaks.py."""
SUITES = ["scrollbar", "scroll_area"]

INPUT = "src/si/scrman_input.c"
AREA = "src/si/area.c"
STACK = "src/si/screen_stack.c"

# name, file, text to find (once in the file), text to put in its place
BREAKS = [
    # FUN_00561f10, the scrollbar click
    ("click: no call", INPUT, "        if (area->has_scrollbar) scrollbar_click(sm, sm->area_order[k]);", ""),
    ("click: shown only", INPUT, "        if (area->has_scrollbar) scrollbar_click(sm, sm->area_order[k]);",
     "        if (area->has_scrollbar && area->shown) scrollbar_click(sm, sm->area_order[k]);"),
    ("click: no second walk", INPUT, "        if (area->shown && area->owner_widget == -1) walk_rows(sm, area, false);", ""),
    ("click: x2", INPUT, "bool across = bar->x1 <= x && x <= bar->x2;", "bool across = bar->x1 <= x && x < bar->x2;"),
    ("click: button", INPUT, "    if (g_left_held && across) {", "    if (across) {"),
    ("up: edge", INPUT, "        if (bar->y1 <= y && y <= bar->track_y1 - 1) {", "        if (bar->y1 <= y && y <= bar->track_y1) {"),
    ("up: floor", INPUT, "bar->position = bar->position - 1 < 1 ? 0 : bar->position - 1;",
     "bar->position = bar->position - 1;"),
    ("up: look", INPUT, "            bar->pressed = 1;\n", ""),
    ("down: ceiling", INPUT, "            s32 position = bar->position + 1;\n\n            bar->position = position < bar->max_position ? position\n",
     "            s32 position = bar->position + 1;\n\n            bar->position = 1 ? position\n"),
    ("down: look", INPUT, "            bar->pressed = 2;\n", "            bar->pressed = 1;\n"),
    ("page up: one row", INPUT, "            s32 position = bar->position - area->row_count;", "            s32 position = bar->position - 1;"),
    ("page down: edge", INPUT, "        } else if (bar->thumb_y2 + 1 <= y && y <= bar->track_y2) {",
     "        } else if (bar->thumb_y2 <= y && y <= bar->track_y2) {"),
    ("page down: ceiling", INPUT, "            bar->position = position < bar->max_position ? position\n                                                         : bar->max_position;\n        } else if (bar->thumb_y1",
     "            bar->position = position;\n        } else if (bar->thumb_y1"),
    ("thumb: no mode", INPUT, "            sm->mode = 2;\n", ""),
    ("thumb: y", INPUT, "            sm->unknown_12f509[1] = y;\n        }", "            sm->unknown_12f509[1] = y + 1;\n        }"),
    ("thumb top", INPUT, "                        bar->position / bar->max_position + bar->track_y1;",
     "                        bar->position / (bar->max_position + 1) + bar->track_y1;"),
    ("look: no redraw", INPUT, "    if (bar->pressed != old_pressed) redraw_scrollbar(area);", ""),
    ("redraw: no restore", INPUT, "        gui_blit(bar->x1, bar->y1, bar->saved);\n", ""),
    ("redraw: no flip", INPUT, "    display_flip(bar->x1, bar->y1, bar->x2, bar->y2);", ""),
    ("click: rows", INPUT, "        area_scroll(area, bar->position - old_position);\n        return 1;",
     "        area_scroll(area, old_position - bar->position);\n        return 1;"),
    # FUN_00561050, the thumb drag
    ("drag: release", INPUT, "    if (!g_left_held) {\n        sm->mode = 0;", "    if (!g_left_held) {\n        sm->mode = 2;"),
    ("drag: top", INPUT, "    if (bar->thumb_y1 + dy < bar->track_y1) dy = bar->track_y1 - bar->thumb_y1;", ""),
    ("drag: bottom", INPUT, "    if (bar->track_y2 < bar->thumb_y2 + dy) dy = bar->track_y2 - bar->thumb_y2;", ""),
    ("drag: position", INPUT, "(bar->track_y2 - bar->thumb_height - bar->track_y1 + 1);",
     "(bar->track_y2 - bar->thumb_height - bar->track_y1);"),
    ("drag: last y", INPUT, "    sm->unknown_12f509[1] = y;\n    return -1;", "    return -1;"),
    ("drag: no scroll", INPUT, "            area_scroll(area, bar->position - old_position);\n        }", "        }"),
    ("poll: no drag", INPUT, "    if (sm->mode == 2) return scrman_scrollbar_drag(sm);", ""),
    # FUN_005621f0, the keys and the wheel
    ("key: none", INPUT, "    if (!bar) return;\n    g_left_held = 1;", "    g_left_held = 1;\n    if (!bar) return;"),
    ("key: frames", INPUT, "    g_click_frames = frames;\n    switch", "    g_click_frames = 1;\n    switch"),
    ("key: down", INPUT, "        g_mouse_y = bar->track_y2 + 2;", "        g_mouse_y = bar->track_y2 + 1;"),
    ("key: page down", INPUT, "        g_mouse_y = bar->thumb_y2 + 2;", "        g_mouse_y = bar->thumb_y2 + 1;"),
    ("key: page up", INPUT, "        g_mouse_y = bar->track_y1 + 1;", "        g_mouse_y = bar->track_y1 + 2;"),
    ("key: other", INPUT, "        g_left_held = 0;\n        g_click_frames = 0;", "        g_click_frames = 0;"),
    ("pump: release", INPUT, "        if (g_click_frames == 0) g_left_held = 0;", ""),
    # FUN_00403600 and FUN_004031a0, the key scrollbar
    ("key bar: not set", AREA, "    g_key_scrollbar = bar;\n", ""),
    ("key bar: not cleared", AREA, "    if (g_key_scrollbar == &area->scrollbar) g_key_scrollbar = NULL;\n", ""),
    # FUN_00403ee0, the scroll
    ("scroll: no restore", AREA, "    if (area->saved) gui_blit(area->x1, area->y1, area->saved);\n", ""),
    ("scroll: no row start", AREA, "        while (first != 0 && list_row(area, first - 1) == list_row(area, first)) {",
     "        while (0) {"),
    ("scroll: up past", AREA, "        while (rows < moved && first - 1 >= 0) {", "        while (rows <= moved && first - 1 >= 0) {"),
    ("scroll: down past", AREA, "        while (moved < rows && first + 1 < area->widget_count) {",
     "        while (moved <= rows && first + 1 < area->widget_count) {"),
    ("scroll: no draw", AREA, "        widget_draw(widget, false);\n    }\n    display_flip", "    }\n    display_flip"),
    ("scroll: flip width", AREA, "    display_flip(area->x1, area->y1, area->x2 - 22, area->y2);",
     "    display_flip(area->x1, area->y1, area->x2, area->y2);"),
    ("scroll: rows in view", AREA, "        if (shown_row > area->row_count - 1) break;\n        place_widget(area, widget, shown_row);\n        widget_draw(widget, false)",
     "        if (shown_row > area->row_count) break;\n        place_widget(area, widget, shown_row);\n        widget_draw(widget, false)"),
    # FUN_004032e0, FUN_00403320 and FUN_00803320, the scroll position
    ("shown row: no scrollbar", AREA, "    if (!area->has_scrollbar) return 0;\n", ""),
    ("scroll to: floor", AREA, "    area->scroll_to = row < 1 ? 0 : row;", "    area->scroll_to = row;"),
    ("scroll to: ceiling", AREA, "    if (row >= last) row = last;", "    if (row > last + 1) row = last;"),
    ("stack row: player", STACK, "    if (rec->player != 0) return area == rec->unknown_22 ? rec->unknown_24 : 0;", ""),
    ("stack row: none", STACK, "    if (area == -1) return 0;\n    return area_shown_row", "    if (area == -1) return 1;\n    return area_shown_row"),
]
