/*
 * Commands of the port driver for text entry (tests/diff/cases_entry.py):
 * the key globals, the caller's text buffer of a widget and one step of
 * scrman_text_entry, on the screen manager of the test scene.
 */
#include "port_entry.h"
#include "port_scene.h"
#include "screen_stack.h"
#include "scrman_input.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define SOURCE_SIZE 512

static widget_t *widget_at(const char *args)
{
    long index = strtol(args, NULL, 10);

    if (index < 0 || index >= SCRMAN_MAX_WIDGETS) return NULL;
    return &scene_manager()->widgets[index];
}

bool entry_command(const char *cmd, const char *args)
{
    long long v[2] = {0, 0};
    int count = sscanf(args, "%lld %lld", &v[0], &v[1]);

    if (!strcmp(cmd, "entry_init")) {
        /* The screen manager of the scene is the one that the screen shows. */
        g_screen_stack.record.screen_manager = scene_manager();
        return true;
    }
    if (!strcmp(cmd, "entry_keys")) {
        if (count != 2) return false;
        g_key_code = (s32)v[0];
        g_key_char = (u8)v[1];
        return true;
    }
    if (!strcmp(cmd, "entry_state")) {
        printf("%d %d %d %d\n", g_key_code, g_key_char, g_cursor_kind,
               g_left_held);
        return true;
    }
    if (!strcmp(cmd, "entry")) {
        if (count != 1) return false;
        bool done = scrman_text_entry(scene_manager(), (s16)v[0]);

        printf("%d %d %d %d %d\n", done, g_key_code, g_key_char,
               g_cursor_kind, g_left_held);
        return true;
    }
    if (!strcmp(cmd, "entry_buffer")) {
        /* "entry_buffer 3 -414243": a new buffer with this text. */
        widget_t *widget = widget_at(args);
        const char *hex = strchr(args, '-');
        char *buffer = calloc(SOURCE_SIZE, 1);
        size_t length = 0;
        unsigned value;

        if (!widget || !hex || !buffer) return false;
        for (hex++; sscanf(hex, "%2x", &value) == 1; hex += 2) {
            if (length == SOURCE_SIZE - 1) return false;
            buffer[length++] = (char)value;
        }
        widget->text_source = buffer;
        return true;
    }
    if (!strcmp(cmd, "entry_text")) {
        widget_t *widget = widget_at(args);

        if (!widget) return false;
        if (!widget->text_source) {
            printf("null\n");
            return true;
        }
        printf("-");
        for (const char *c = widget->text_source; *c; c++) {
            printf("%02x", (unsigned char)*c);
        }
        printf("\n");
        return true;
    }
    return false;
}
