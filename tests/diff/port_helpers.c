/*
 * Commands of the port driver for the helpers of the screens
 * (tests/diff/cases_helpers.py): the text function, the widget helpers, the
 * menu builder, the Back and Next bar and the text box, on g_screen_stack with
 * the screen manager of the test scene.
 */
#include "port_helpers.h"
#include "gui_menu.h"
#include "langlib.h"
#include "port_scene.h"
#include "screen_stack.h"
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define TEXTS 16

long next_long(const char **args)
{
    char *end;
    long value = strtol(*args, &end, 10);

    *args = end;
    return value;
}

char *next_text(const char **args)
{
    static char texts[TEXTS][TEXT_SIZE];
    static int turn;
    const char *p = *args;
    char *text = texts[turn++ % TEXTS];
    size_t length = 0;
    unsigned value;

    while (*p == ' ') p++;
    if (!strncmp(p, "null", 4)) {
        *args = p + 4;
        return NULL;
    }
    if (*p == '-') p++;
    /* sscanf skips spaces before a number, so each pair is checked first. */
    while (isxdigit((unsigned char)p[0]) && isxdigit((unsigned char)p[1]) &&
           sscanf(p, "%2x", &value) == 1 && length < TEXT_SIZE - 1) {
        text[length++] = (char)value;
        p += 2;
    }
    text[length] = '\0';
    *args = p;
    return text;
}

void print_hex(const char *text)
{
    printf("-");
    for (const char *c = text; c && *c; c++) printf("%02x", (unsigned char)*c);
}

/* "st_text -format count kind value ...": kind d is a number, s a text. */
static void run_text(const char *args)
{
    char *format = next_text(&args);
    long count = next_long(&args);
    intptr_t values[4] = {0};
    char *strings[4] = {0};
    char result[TEXT_SIZE + 64];

    for (long i = 0; i < count && i < 4; i++) {
        while (*args == ' ') args++;
        if (*args++ == 's') {
            strings[i] = next_text(&args);
        } else {
            values[i] = next_long(&args);
        }
    }
    /* The tests use at most four arguments, each a text or an int. */
#define ARG(i) (strings[i] ? (intptr_t)strings[i] : values[i])
    memset(result, 0, sizeof(result));
    char *end = text_format(result, format, ARG(0), ARG(1), ARG(2), ARG(3));
#undef ARG
    printf("%ld ", (long)(end - result));
    print_hex(result);
    printf("\n");
}

/* "st_menu x1 y1 x2 y2 columns rows font count mask sort", then font colour id data -text for each item. */
static void run_menu(const char *args)
{
    static gui_menu_item_t items[40];
    long v[10];

    for (int i = 0; i < 10; i++) v[i] = next_long(&args);
    for (long i = 0; i < v[7] && i < 40; i++) {
        gui_menu_item_t *item = &items[i];
        char *text;

        memset(item, 0, sizeof(*item));
        item->font = (s16)next_long(&args);
        item->colour = (u16)next_long(&args);
        item->id = (s32)next_long(&args);
        item->data = (s32)next_long(&args);
        text = next_text(&args);
        snprintf(item->text, sizeof(item->text), "%s", text ? text : "");
    }
    printf("%d\n", gui_menu_build((s32)v[0], (s32)v[1], (s32)v[2], (s32)v[3],
                                  (s32)v[4], (s32)v[5], (s32)v[6], (s32)v[7],
                                  v[7] > 0 ? items : NULL, (u32)v[8],
                                  (s32)v[9]));
}

bool helpers_command(const char *cmd, const char *args)
{
    screen_stack_t *st = &g_screen_stack;

    if (!strcmp(cmd, "st_init")) {
        screen_stack_free_all(st);
        screen_stack_init(st, scene_manager());
    } else if (!strcmp(cmd, "st_text")) {
        run_text(args);
    } else if (!strcmp(cmd, "st_heading")) {
        long colour = next_long(&args), text_colour = next_long(&args);

        printf("%d\n", screen_heading(st, (u16)colour, (u16)text_colour,
                                      next_text(&args)));
    } else if (!strcmp(cmd, "st_sub")) {
        printf("%d\n", screen_sub_heading(st, next_text(&args)));
    } else if (!strcmp(cmd, "st_cell")) {
        /* "st_cell button area column row style colour flags font colour id data -text" */
        long v[11];

        for (int i = 0; i < 11; i++) v[i] = next_long(&args);
        char *text = next_text(&args);

        if (v[0]) {
            printf("%d\n", screen_cell_button(st, (s16)v[1], (s32)v[2], (s32)v[3],
                                              (u32)v[4], (u16)v[5], (u32)v[6],
                                              (s16)v[7], (u16)v[8], text,
                                              (s32)v[9], (intptr_t)v[10]));
        } else {
            printf("%d\n", screen_cell_label(st, (s16)v[1], (s32)v[2], (s32)v[3],
                                             (u32)v[4], (u16)v[5], (u32)v[6],
                                             (s16)v[7], (u16)v[8], text));
        }
    } else if (!strcmp(cmd, "st_free")) {
        long v[11];

        for (int i = 0; i < 11; i++) v[i] = next_long(&args);
        printf("%d\n", screen_free_button(st, (s32)v[0], (s32)v[1], (s32)v[2],
                                          (s32)v[3], (u32)v[4], (u16)v[5],
                                          (u32)v[6], (s16)v[7], (u16)v[8],
                                          next_text(&args), (s32)v[9],
                                          (intptr_t)v[10]));
    } else if (!strcmp(cmd, "st_menu")) {
        run_menu(args);
    } else if (!strcmp(cmd, "st_back_next")) {
        long back = next_long(&args), next = next_long(&args);

        printf("%d\n", gui_back_next_bar((s32)back, (s32)next));
    } else if (!strcmp(cmd, "st_text_box")) {
        long v[9];

        for (int i = 0; i < 9; i++) v[i] = next_long(&args);
        printf("%d\n", gui_text_box((s32)v[0], (s32)v[1], (s32)v[2], (s32)v[3],
                                    (u32)v[4], (u16)v[5], (s32)v[6], (s16)v[7],
                                    (u16)v[8], next_text(&args)));
    } else {
        return false;
    }
    return true;
}
