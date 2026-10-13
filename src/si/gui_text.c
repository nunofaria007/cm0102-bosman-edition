#include "gui_text.h"
#include "display.h"
#include "gui_font.h"
#include "gui_utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define LINE_SIZE 252
#define REST_SIZE LINE_SIZE
#define MAX_LINES (2 * LINE_SIZE)

typedef struct {
    char *lines[MAX_LINES];
    s32 count;
} line_list_t;

static void add_line(line_list_t *list, const char *line)
{
    size_t size = strlen(line) + 1;
    char *copy = malloc(size);

    if (!copy) return;
    memcpy(copy, line, size);
    list->lines[list->count++] = copy;
}

static void cut_line(char *line, s32 at, s32 rest_from, char *rest)
{
    snprintf(rest, REST_SIZE, "%s", line + rest_from);
    line[at] = '\0';
}

static void split_lines(line_list_t *list, const char *text, u32 flags,
                        s16 font, s32 box_width)
{
    char line[LINE_SIZE] = "";
    char rest[REST_SIZE] = "";
    s32 length = (s32)strnlen(text, LINE_SIZE - 1);
    bool cut = false;

    for (s32 i = 1; i <= length; i++) {
        u8 c = (u8)text[i - 1];

        if (c >= ' ' && !cut) {
            /* "#", four more characters and "#" stay together, as a code. */
            bool group = c == '#' && i < length - 4 && text[i + 4] == '#';
            s32 piece = group ? 6 : 1;

            strncat(line, text + i - 1, (size_t)piece);
            i += piece - 1;
            if (gui_line_width(font, line) > box_width) {
                s32 space = i - 1;

                if (i > 6 && line[i - 1] == '#') {
                    cut_line(line, i - 6, i - 6, rest);
                } else {
                    while (i > 1 && line[space] != ' ' && --space > 0) {
                    }
                    cut_line(line, space, space + 1, rest);
                }
                cut = true;
            }
        }
        if (text[i - 1] == '\n' || ((flags & GUI_TEXT_WRAP) && cut)) {
            add_line(list, line);
            strcpy(line, rest);
            rest[0] = '\0';
            cut = false;
        }
        if (i == length && line[0] != '\0') add_line(list, line);
    }
}

void gui_text(s32 x1, s32 y1, s32 x2, s32 y2, u32 flags, s16 font, u16 colour,
              const char *text, s32 cursor)
{
    line_list_t list = { { NULL }, 0 };
    s32 box_width = x2 - x1 + 1;
    s32 box_height = y2 - y1 + 1;
    s32 line_height;
    s32 y = y1;

    if (font < 0 || font >= GUI_FONT_COUNT || !text) return;
    line_height = g_fonts[font].height;

    split_lines(&list, text, flags, font, box_width);
    if (cursor > -1 && list.count == 0) add_line(&list, "");
    if (!(flags & GUI_TEXT_TOP)) {
        y = (box_height - list.count * line_height) / 2 + y1;
    }

    for (s32 i = 0; i < list.count; i++, y += line_height) {
        char *line = list.lines[i];
        u16 line_colour = colour;
        s32 x;

        if (flags & GUI_TEXT_PASSWORD) memset(line, '*', strlen(line));
        if (flags & GUI_TEXT_LEFT) {
            x = x1;
        } else if (flags & GUI_TEXT_RIGHT) {
            x = x2 - gui_line_width(font, line) + 1;
        } else {
            x = (box_width - gui_line_width(font, line)) / 2 + x1;
        }
        if (flags & GUI_TEXT_EMBOSS) {
            s32 centre_x = box_width / 2 + x1;
            s32 centre_y = box_height / 2 + y1;
            gui_rect_t c;
            u16 under = 0;

            if (gui_clip(centre_x, centre_y, centre_x, centre_y, &c)) {
                under = display_get_pixel(centre_x, centre_y);
            }
            gui_draw_line(x + 1, y + 1, font, gui_scale_colour(under, 166),
                          line, cursor);
            line_colour = gui_scale_colour(under, 66);
        }
        gui_draw_line(x, y, font, line_colour, line, cursor);
        free(line);
    }
}
