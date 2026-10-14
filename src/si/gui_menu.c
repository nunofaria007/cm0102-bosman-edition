#include "gui_menu.h"
#include "gui_font.h"
#include "gui_utils.h"
#include "langlib.h"
#include "screen_stack.h"
#include <stdlib.h>
#include <string.h>

static scrman_t *screen_manager(void)
{
    return g_screen_stack.record.screen_manager;
}

void gui_menu_item_init(gui_menu_item_t *item)
{
    item->font = -1;
    item->colour = 0;
}

/* The sort of FUN_005ed300. A swap also swaps the two bits of the mask. */
static u32 sort_items(gui_menu_item_t *items, s32 count, u32 mask)
{
    for (s32 i = 0; i < count - 1; i++) {
        for (s32 j = i + 1; j < count; j++) {
            if (strcmp(items[i].text, items[j].text) <= 0) continue;

            gui_menu_item_t swap = items[i];
            u32 bit_i = 1u << (i & 31), bit_j = 1u << (j & 31);

            items[i] = items[j];
            items[j] = swap;
            if ((mask & bit_i) && !(mask & bit_j)) {
                mask += bit_j - bit_i;
            } else if (!(mask & bit_i) && (mask & bit_j)) {
                mask += bit_i - bit_j;
            }
        }
    }
    return mask;
}

s32 gui_menu_build(s32 x1, s32 y1, s32 x2, s32 y2, s32 columns, s32 rows,
                   s32 default_font, s32 count, const gui_menu_item_t *items,
                   u32 bold_mask, s32 sort)
{
    s32 per_row = columns & 0xff;

    /* The original shows an error box here (gui_utils.cpp 193). */
    if (count <= 0 || !items || per_row == 0) return 0;

    gui_menu_item_t *copy = malloc(sizeof(*copy) * (size_t)count);

    if (!copy) return 0;
    memcpy(copy, items, sizeof(*copy) * (size_t)count);
    if (sort) bold_mask = sort_items(copy, count, bold_mask);

    scrman_t *sm = screen_manager();
    s16 area = scrman_area_create(sm, x1, y1, x2, y2, (u8)columns, NULL,
                                  (u8)rows, NULL, 1, 0, -1);
    s32 column = 0, row = 0, wx1 = 0, wx2 = 0, width = 0;
    s32 left = count;

    for (s32 k = 0; k < count; k++, left--) {
        const gui_menu_item_t *item = &copy[k];
        u32 style;
        u16 text_colour;

        /* Only a last row with fewer items than columns is centred. */
        if (k % per_row == 0 && left < per_row) {
            width = (x2 - 2 * per_row - x1 + 3) / per_row;
            wx1 = (x2 - (width + 2) * left - x1 + 3) / 2 + x1;
            wx2 = width + wx1 - 1;
        }
        if (bold_mask & (1u << (k & 31))) {
            style = 0x822;
            text_colour = item->colour ? item->colour : GUI_COLOUR_YELLOW;
        } else {
            style = 0x22;
            text_colour = item->colour ? item->colour : GUI_COLOUR_LIGHT_GREY;
        }
        scrman_widget_create(sm, 2, wx1, 0, wx2, 0, column, row, style,
                             GUI_COLOUR_NAVY, GUI_COLOUR_NAVY, 0xc,
                             item->font == -1 ? (s16)default_font : item->font,
                             text_colour, item->text, 0, item->id, item->data,
                             area);
        if (wx1 != 0 && wx2 != 0) {
            wx1 += width + 2;
            wx2 += width + 2;
        }
        if (++column == per_row) {
            column = 0;
            row++;
        }
    }
    free(copy);
    return 1;
}

s32 gui_back_next_bar(s32 back, s32 next)
{
    static const u8 weights[2] = {3, 1};
    screen_stack_t *st = &g_screen_stack;
    char text[0x7d0];
    s16 area = scrman_area_create(screen_manager(), 100, 555, 790, 590, 2,
                                  weights, 1, NULL, 1, 0, -1);

    text_format(text, "Back");
    if (back) {
        screen_cell_button(st, area, 0, 0, 0x30, GUI_COLOUR_GREY, 0xc, 3,
                           GUI_COLOUR_LIGHT_GREY, text, -2, 0);
    } else {
        screen_cell_label(st, area, 0, 0, 0x30, GUI_COLOUR_GREY, 0x2c, 3,
                          GUI_COLOUR_LIGHT_GREY, text);
    }
    text_format(text, "Next");
    if (next) {
        screen_cell_button(st, area, 1, 0, 0x30, GUI_COLOUR_GREY, 0xc, 3,
                           GUI_COLOUR_LIGHT_GREY, text, -3, 0);
    } else {
        screen_cell_label(st, area, 1, 0, 0x30, GUI_COLOUR_GREY, 0x2c, 3,
                          GUI_COLOUR_LIGHT_GREY, text);
    }
    return 1;
}

/*
 * FUN_004aeaf0 of comp_text.cpp, for a text that fits on one line: a label
 * in row `row` of the area, centred between x1 and x2 by its width.
 */
static void text_line(s16 area, s32 x1, s32 x2, s32 row, s16 font,
                      u16 text_colour, const char *text)
{
    s32 width = gui_line_width(font, text);
    s32 left = x1 + ((x2 - x1 + 1) - width) / 2;

    scrman_widget_create(screen_manager(), 1, left, 0, left - 1 + width, 0, 0,
                         row, 1, 0, 0, 1, font, text_colour, text, 0, 0, 0,
                         area);
}

s32 gui_text_box(s32 x1, s32 y1, s32 x2, s32 y2, u32 flags, u16 colour,
                 s32 rows, s16 font, u16 text_colour, const char *text)
{
    s16 area = scrman_area_create(screen_manager(), x1, y1, x2, y2, 1, NULL,
                                  (u8)rows, NULL, flags, colour, -1);

    if (!text) return 1;
    if (gui_line_width(font, text) <= x2 - x1 - 9 && !strchr(text, '\n')) {
        text_line(area, x1 + 5, x2 - 5, (rows - 1) / 2, font,
                  text_colour, text);
    }
    return 1;
}
