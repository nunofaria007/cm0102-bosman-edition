"""Deliberate breaks of the progress box and the local part of opcode 7. Each one must fail a suite.
See tools/breaks.py."""
SUITES = ["progress_box", "progress_opcode_7"]

B = "src/si/progress_box.c"
S = "src/si/scrman.c"

# Equivalent: text flags 0xc and 0x4 draw the same, since the text function reads neither 0x4 nor 0x8.
# name, file, text to find (once in the file), text to put in its place
BREAKS = [
    ("no pump", B, "    if (g_screen_pump) g_screen_pump();         /* FUN_0068d520 */\n", ""),
    ("pump after null", B, "    if (g_screen_pump) g_screen_pump();         /* FUN_0068d520 */\n    if (!text) return 0;",
     "    if (!text) return 0;\n    if (g_screen_pump) g_screen_pump();"),
    ("null draws", B, "    if (!text) return 0;", "    if (!text) text = \"\";"),
    ("darken on step 1", B, "    if (step == 0 && g_screen_stack", "    if (step == 1 && g_screen_stack"),
    ("no darken", B, "        scrman_darken_once(g_screen_stack.record.screen_manager);\n", ""),
    ("outer style", B, "GUI_PANEL_FILL | GUI_PANEL_BORDER,", "GUI_PANEL_FILL,"),
    ("outer box", B, "gui_panel(100, 555, 790, 590,", "gui_panel(100, 555, 790, 591,"),
    ("total zero", B, "if (total > 0 && step <= total)", "if (total >= 0 && step <= total)"),
    ("step equal", B, "if (total > 0 && step <= total)", "if (total > 0 && step < total)"),
    ("no wrap", B, "(s32)((u32)step * BAR_WIDTH) / total", "(s32)((long long)step * BAR_WIDTH / total)"),
    ("bar width", B, "#define BAR_WIDTH 496", "#define BAR_WIDTH 498"),
    ("bar start", B, "#define BAR_X1    272", "#define BAR_X1    271"),
    ("bar bottom", B, "gui_rect(BAR_X1, 567, end, 578,", "gui_rect(BAR_X1, 567, end, 577,"),
    ("bar colour", B, "GUI_RECT_FILL, GUI_COLOUR_NAVY", "GUI_RECT_FILL, GUI_COLOUR_BLACK"),
    ("bar panel", B, "GUI_PANEL_BORDER | GUI_PANEL_SUNKEN,", "GUI_PANEL_BORDER,"),
    ("narrow text box", B, "gui_text(120, 565, 250, 580,", "gui_text(120, 565, 260, 580,"),
    ("wide text box", B, "gui_text(120, 565, 770, 580,", "gui_text(120, 565, 250, 580,"),
    ("text flags", B, "gui_text(120, 565, 250, 580, 0xc,", "gui_text(120, 565, 250, 580, 0xd,"),
    ("text colour", B, "gui_text(120, 565, 770, 580, 0xc, 1, GUI_COLOUR_LIGHT_GREY", "gui_text(120, 565, 770, 580, 0xc, 1, GUI_COLOUR_GREY"),
    ("flip box", B, "display_flip(100, 555, 790, 590);", "display_flip(100, 555, 790, 589);"),
    ("result", B, "    display_flip(100, 555, 790, 590);\n    return 1;", "    display_flip(100, 555, 790, 590);\n    return 0;"),
    ("darken twice", S, "    if (sm->darken_screen) return;\n", ""),
    ("darken flag", S, "    sm->darken_screen = 1;\n    gui_darken(0, 0, 799, 599);", "    gui_darken(0, 0, 799, 599);"),
    ("darken flip", S, "    gui_darken(0, 0, 799, 599);\n    display_flip(0, 0, 799, 599);\n}", "    gui_darken(0, 0, 799, 599);\n}"),
    ("no records", B, "if (st->record_count <= 0 || st->record.player != 0) return;", "if (st->record.player != 0) return;"),
    ("channel 0 off", B, "    } else if (channel == 0) {\n        if (st->record.player != 0) return;",
     "    } else if (channel == 0) {\n        return;"),
    ("other channels off", B, "    } else if (channel < 1 || channel > 15) {", "    } else {"),
    ("12f551 kept", B, "    st->record.screen_manager->unknown_12f551 = 0;\n", ""),
]
