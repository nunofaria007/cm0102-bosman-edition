"""Deliberate breaks of the mode 0 input. Each one must fail a suite. See tools/breaks.py."""
SUITES = ["hit"]

# name, file, text to find (once in the file), text to put in its place
BREAKS = [
    ("right flag", "src/si/scrman_input.c", "s32 held = (w->flags & WIDGET_RIGHT_CLICK) ? g_right_held : g_left_held;", "s32 held = g_left_held;"),
    ("inside x2", "src/si/scrman_input.c", "g_mouse_x <= w->x2", "g_mouse_x < w->x2"),
    ("inside y1", "src/si/scrman_input.c", "w->y1 <= g_mouse_y", "w->y1 < g_mouse_y"),
    ("off: highlight kept", "src/si/scrman_input.c", "        if (w->highlighted) widget_set_highlight(w, false);\n", ""),
    ("cursor: released", "src/si/scrman_input.c", "        if (inside && (w->unknown_08 != 0 || w->popup_area != -1)) {", "        if (0) {"),
    ("on: always", "src/si/scrman_input.c", "    if (!w->highlighted) widget_set_highlight(w, true);", "    widget_set_highlight(w, true);"),
    ("button flag", "src/si/scrman_input.c", "        if ((w->flags & WIDGET_BUTTON) && test_widget(w)) hit = index;\n    }\n    for", "        if (test_widget(w)) hit = index;\n    }\n    for"),
    ("first wins", "src/si/scrman_input.c", "        if ((w->flags & WIDGET_BUTTON) && test_widget(w)) hit = index;\n    }\n    for", "        if ((w->flags & WIDGET_BUTTON) && test_widget(w) && hit == -1) hit = index;\n    }\n    for"),
    ("areas: hidden", "src/si/scrman_input.c", "        if (!area->shown || area->owner_widget != -1) continue;", "        if (area->owner_widget != -1) continue;"),
    ("areas: skipped", "src/si/scrman_input.c", "            if ((w->flags & WIDGET_BUTTON) && test_widget(w)) hit = index;\n        }", "            (void)w;\n        }"),
    ("drag: mode", "src/si/scrman_input.c", "        sm->mode = 3;\n", ""),
    ("drag: return", "src/si/scrman_input.c", "        sm->unknown_12f521 = NULL;\n        return -1;", "        sm->unknown_12f521 = NULL;\n        return hit;"),
    ("drag: box", "src/si/scrman_input.c", "        sm->unknown_12f509[4] = w->x2;", "        sm->unknown_12f509[4] = w->x1;"),
    ("highlight: time", "src/si/guio.c", "    widget->tick = g_tick;\n    if ((widget->style", "    if ((widget->style"),
    ("highlight: style", "src/si/guio.c", "    if ((widget->style & (GUI_PANEL_FILL | GUI_PANEL_BORDER)) &&", "    if ((widget->style & GUI_PANEL_BORDER) &&"),
    ("highlight: raised", "src/si/guio.c", "        !((widget->flags & WIDGET_STAYS_RAISED) &&\n          widget->colour == widget->highlight_colour)) {", "        1) {"),
    ("highlight: no restore", "src/si/guio.c", "        if (widget->saved) gui_blit(widget->x1, widget->y1, widget->saved);\n", ""),
    ("highlight: no flip", "src/si/guio.c", "        display_flip(widget->x1, widget->y1, widget->x2, widget->y2);\n    }\n    if (widget->area == -1) return;", "    }\n    if (widget->area == -1) return;"),
    ("highlight: area on", "src/si/guio.c", "        area->unknown_210 = widget->index;", "        area->unknown_210 = -1;"),
    ("highlight: area off", "src/si/guio.c", "    } else if (area->unknown_210 == widget->index) {", "    } else {"),
]
