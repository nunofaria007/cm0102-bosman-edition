"""Deliberate breaks of the key, wheel and mouse mapping. Each one must fail a suite. See tools/breaks.py."""
SUITES = ["window"]

# End and Home both do nothing in the original, so a swap of modes 4 and 5 is equivalent.
# name, file, text to find (once in the file), text to put in its place
BREAKS = [
    ("page up as page down", "src/si/input.c", "    case SDLK_PAGEUP: return 3;", "    case SDLK_PAGEUP: return 2;"),
    ("up as down", "src/si/input.c", "    case SDLK_UP: return 1;", "    case SDLK_UP: return 0;"),
    ("wheel reversed", "src/si/input.c", "scrman_scroll_key(e->wheel.y > 0 ? 1 : 0,", "scrman_scroll_key(e->wheel.y > 0 ? 0 : 1,"),
    ("wheel one frame", "src/si/input.c", "                              e->wheel.y > 0 ? e->wheel.y : -e->wheel.y);", "                              1);"),
    ("move lock", "src/si/input.c", "        if (mouse_free) set_mouse(e->motion.x, e->motion.y);", "        set_mouse(e->motion.x, e->motion.y);"),
    ("button lock", "src/si/input.c", "    if (mouse_free &&\n        (e->type == SDL_MOUSEBUTTONDOWN", "    if (1 &&\n        (e->type == SDL_MOUSEBUTTONDOWN"),
    ("left click flag", "src/si/input.c", "            if (down) g_left_clicked = 1;", "            (void)0;"),
    ("right held", "src/si/input.c", "            g_right_held = down;", "            g_right_held = 0;"),
]
