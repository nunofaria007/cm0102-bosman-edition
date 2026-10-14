"""Deliberate breaks of the screen draw and the fade. Each one must fail a suite. See tools/breaks.py."""
SUITES = ["fade", "screen_draw"]

# name, file, text to find (once in the file), text to put in its place
BREAKS = [
    ("fade: no frame", "src/si/gui_utils.c", "for (s32 frame = 0; frame <= frames; frame++)", "for (s32 frame = 0; frame < frames; frame++)"),
    ("fade: no reverse", "src/si/gui_utils.c", "shades[colour][steps < 0 ? frames - k : k] = shade;", "shades[colour][k] = shade;"),
    ("fade: percent", "src/si/gui_utils.c", "(u8)(k * 100 / frames)", "(u8)(k * 99 / frames)"),
    ("fade: zero steps", "src/si/gui_utils.c", "    if (steps == 0 || !gui_clip(x1, y1, x2, y2, &c)) return;", "    if (!gui_clip(x1, y1, x2, y2, &c) || steps == 0) { display_flip(0, 0, 0, 0); return; }"),
    ("fade: clip", "src/si/gui_utils.c", "    if (steps == 0 || !gui_clip(x1, y1, x2, y2, &c)) return;", "    if (steps == 0) return;\n    gui_clip(x1, y1, x2, y2, &c);"),
    ("fade: flip box", "src/si/gui_utils.c", "display_flip(c.x1, c.y1, c.x2, c.y2);", "display_flip(c.x1, c.y1, c.x2, c.y2 + 1);"),
    ("draw: skip", "src/si/scrman.c", "    if (sm->skip_draw) return;\n", ""),
    ("draw: no backgrounds", "src/si/scrman.c", "    if (g_no_backgrounds && background[0] != '\\0') background = \"\\\\\";\n", ""),
    ("draw: no backgrounds empty", "src/si/scrman.c", "if (g_no_backgrounds && background[0] != '\\0')", "if (g_no_backgrounds)"),
    ("draw: case", "src/si/scrman.c", "if (!strcasecmp(sm->background_name, name)) return;", "if (!strcmp(sm->background_name, name)) return;"),
    ("draw: same name", "src/si/scrman.c", "    if (!strcasecmp(sm->background_name, name)) return;\n", ""),
    ("draw: empty capture", "src/si/scrman.c", "gui_capture(0, 0, 799, 599, sm->background);", "gui_capture(0, 0, 799, 598, sm->background);"),
    ("draw: empty name kept", "src/si/scrman.c", "        sm->background_name[0] = '\\0';\n        return;", "        return;"),
    ("draw: no default", "src/si/scrman.c", "        img = rgn_load_into(path, sm->background);\n", ""),
    ("draw: no reuse", "src/si/scrman.c", "    img = rgn_load_into(sm->background_name, sm->background);", "    img = rgn_load_into(sm->background_name, NULL);"),
    ("draw: keep old", "src/si/scrman.c", "        rgn_free(sm->background);\n        sm->background = img;", "        sm->background = img;"),
    ("draw: no blit", "src/si/scrman.c", "    if (sm->background) gui_blit(0, 0, sm->background);\n", ""),
    ("draw: never layout", "src/si/scrman.c", "    if (!sm->laid_out) {", "    if (0) {"),
    ("draw: plain shown", "src/si/scrman.c", "if (area->shown && area->owner_widget == -1) area_draw(area, true);", "if (area->owner_widget == -1) area_draw(area, true);"),
    ("draw: popups first", "src/si/scrman.c", "if (area->shown && area->owner_widget != -1) area_draw(area, true);", "if (0) area_draw(area, true);"),
    ("draw: no widgets", "src/si/scrman.c", "        widget_draw(&sm->widgets[sm->widget_order[i]], true);", "        (void)i;"),
    ("draw: darken alone", "src/si/scrman.c", "if (sm->darken_screen && g_darken_screens)", "if (sm->darken_screen)"),
    ("draw: fade kept", "src/si/scrman.c", "        sm->fade_in = 0;\n", ""),
    ("draw: no fade", "src/si/scrman.c", "        gui_fade(0, 0, 799, 599, 20);", "        ;"),
    ("draw: flip box", "src/si/scrman.c", "        display_flip(x1, y1, x2, y2);", "        display_flip(x1, y1, x2, y2 - 1);"),
    ("draw: flip test", "src/si/scrman.c", "if (x1 == -1 || x2 == -1 || y1 == -1 || y2 == -1) {", "if (x1 == -1 || x2 == -1 || y1 == -1) {"),
    ("draw: laid out", "src/si/scrman.c", "    sm->laid_out = 1;\n}", "}"),
    ("load: reuse size", "src/si/rgn_loader.c", "if (reuse && reuse->width == w && reuse->height == h) {", "if (reuse && reuse->width == w) {"),
]
