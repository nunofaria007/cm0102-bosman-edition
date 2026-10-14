"""Deliberate breaks of the screen stack. Each one must fail a suite. See tools/breaks.py."""
SUITES = ["stack", "stack_limit"]

# name, file, text to find (once in the file), text to put in its place
BREAKS = [
    ("default: key", "src/si/screen_stack.c", "if (!rec->key && !rec->first && st->default_build) {", "if (!rec->first && st->default_build) {"),
    ("default: first", "src/si/screen_stack.c", "if (!rec->key && !rec->first && st->default_build) {", "if (!rec->key && st->default_build) {"),
    ("no input", "src/si/screen_stack.c", "    if (!build || !input) return 0;", "    if (!build) return 0;"),
    ("flagged: return", "src/si/screen_stack.c", "if (rec->current && flag && rec->first && return_to_flagged(rec, build)) {", "if (0) {"),
    ("flagged: last flagged", "src/si/screen_stack.c", "            if (node->flag) rec->current = node;", "            if (node->flag) { rec->current = node; break; }"),
    ("flagged: restore", "src/si/screen_stack.c", "        if (rec->current->build != build) rec->current = old;\n", ""),
    ("forward: keep", "src/si/screen_stack.c", "    if (rec->current != rec->last) {", "    if (0) {"),
    ("limit", "src/si/screen_stack.c", "    if (rec->count > SCREEN_MAX_NODES) {", "    if (rec->count >= SCREEN_MAX_NODES) {"),
    ("limit: flag", "src/si/screen_stack.c", "        if (node->flag) return 0;\n", ""),
    ("leave", "src/si/screen_stack.c", "    if (rec->current && rec->current->leave) rec->current->leave();\n\n    node = calloc", "\n    node = calloc"),
    ("title", "src/si/screen_stack.c", '    strcpy(node->title, "\\\\");', '    strcpy(node->title, "");'),
    ("flagged count", "src/si/screen_stack.c", "    if (flag) rec->flagged++;\n", ""),
    ("away", "src/si/screen_stack.c", "    rec->away = 0;\n", ""),
    ("done", "src/si/screen_stack.c", "    if (flag) rec->done = 0;\n", "    rec->done = 0;\n"),
    ("rebuild", "src/si/screen_stack.c", "    rec->rebuild = 1;\n    if (used_default)", "    if (used_default)"),
    ("default slots", "src/si/screen_stack.c", "            screen_stack_set_slot(st, i, st->default_values[i], false);", "            screen_stack_set_slot(st, i + 1, st->default_values[i], false);"),
    ("free all: flagged", "src/si/screen_stack.c", "    rec->count = 0;\n    rec->flagged = 0;", "    rec->count = 0;"),
    ("remove: leave", "src/si/screen_stack.c", "    if (rec->current && rec->current->leave) rec->current->leave();\n    screen_stack_free_all(st);", "    screen_stack_free_all(st);"),
    ("remove: push", "src/si/screen_stack.c", "    screen_stack_push(st, st->default_build, st->default_input,", "    if (0) screen_stack_push(st, st->default_build, st->default_input,"),
    ("remove: rebuild", "src/si/screen_stack.c", "    memset(rec, 0, sizeof(*rec));\n    rec->rebuild = 1;", "    memset(rec, 0, sizeof(*rec));"),
    ("slot: copy", "src/si/screen_stack.c", "    if (value && copy) {", "    if (copy) {"),
    ("slot: range", "src/si/screen_stack.c", "if (!node || slot < 0 || slot >= SCREEN_SLOTS) return;", "if (!node || slot < 0 || slot > SCREEN_SLOTS) return;"),
    ("get slot: range", "src/si/screen_stack.c", "if (!node || slot < 0 || slot >= SCREEN_SLOTS) return 0;", "if (!node || slot >= SCREEN_SLOTS) return 0;"),
    ("title: null", "src/si/screen_stack.c", 'title ? title : ""', 'title ? title : "\\\\"'),
    ("go back: any", "src/si/screen_stack.c", "if (node && node->prev && (any || !node->flag)) return 0;", "if (node && node->prev && !node->flag) return 0;"),
    ("at end", "src/si/screen_stack.c", "    return st->record.current->next == NULL;", "    return st->record.current->prev == NULL;"),
]
