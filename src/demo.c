#include "demo.h"
#include "area.h"
#include "gui_text.h"
#include "gui_utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

extern cm_image_t *rgn_load(const char *path);
extern void rgn_free(cm_image_t *img);
extern char *os_find_file_ci(const char *dir, const char *name);

#define TABLE_COLUMNS 4
#define TABLE_ROWS    10
#define TEAM_COUNT    16
#define TABLE_WIDGETS (TEAM_COUNT * TABLE_COLUMNS)

#define COLOUR_WHITE     0xffff
#define COLOUR_DARK_BLUE 0x0129

static const char *const g_teams[TEAM_COUNT] = {
    "Arsenal", "Aston Villa", "Blackburn Rovers", "Bolton Wanderers",
    "Charlton Athletic", "Chelsea", "Derby County", "Everton", "Fulham",
    "Ipswich Town", "Leeds United", "Leicester City", "Liverpool",
    "Manchester United", "Middlesbrough", "Newcastle United",
};

static area_t g_table;
static widget_t g_widgets[TABLE_WIDGETS + 4];

static widget_t *add_widget(s32 index, const char *text, s16 font, u32 style,
                            u16 colour, u32 text_flags)
{
    widget_t *widget = &g_widgets[index];

    widget->areas = &g_table;
    widget->style = style;
    widget->colour = colour;
    widget->highlight_colour = GUI_COLOUR_YELLOW;
    widget->font = font;
    widget->text_colour = style ? GUI_COLOUR_BLACK : COLOUR_WHITE;
    widget->text_flags = text_flags;
    widget->cursor = -1;
    widget->area = -1;
    widget->popup_area = -1;
    snprintf(widget->text, sizeof(widget->text), "%s", text);
    return widget;
}

static void place(widget_t *widget, s32 x1, s32 y1, s32 x2, s32 y2)
{
    widget->x1 = x1;
    widget->y1 = y1;
    widget->x2 = x2;
    widget->y2 = y2;
}

static void build_table(void)
{
    static const u8 weights[TABLE_COLUMNS] = { 6, 1, 1, 1 };

    g_table.areas = &g_table;
    g_table.widgets = g_widgets;
    g_table.x1 = 140;
    g_table.y1 = 130;
    g_table.x2 = 660;
    g_table.y2 = 470;
    g_table.flags = GUI_PANEL_FILL | GUI_PANEL_BORDER;
    g_table.colour = COLOUR_DARK_BLUE;
    g_table.owner_widget = -1;
    g_table.scroll_to = -1;
    g_table.shown = 1;
    g_table.column_count = TABLE_COLUMNS;
    g_table.row_count = TABLE_ROWS;
    g_table.last_row = TEAM_COUNT - 1;
    memcpy(g_table.column_weight, weights, sizeof(weights));
    memset(g_table.row_weight, 1, TABLE_ROWS);

    for (s32 team = 0; team < TEAM_COUNT; team++) {
        s32 points = 3 * (TEAM_COUNT - team) + team % 3;

        for (s32 column = 0; column < TABLE_COLUMNS; column++) {
            s32 index = team * TABLE_COLUMNS + column;
            char number[16];
            widget_t *widget;

            snprintf(number, sizeof(number), "%d",
                     column == 1 ? 38 : column == 2 ? points / 3 : points);
            widget = add_widget(index, column == 0 ? g_teams[team] : number, 3,
                                0, 0, column == 0 ? GUI_TEXT_LEFT : 0);
            widget->area = 0;
            widget->column = column;
            widget->row = team;
            g_table.widget_index[g_table.widget_count++] = (s16)index;
        }
    }
}

void demo_draw(const char *data_dir, const char *background)
{
    char *default_path = os_find_file_ci(data_dir, "default_pic.rgn");
    cm_image_t *picture = rgn_load(background ? background : default_path);
    widget_t *widget;

    gui_blit(0, 0, picture);
    rgn_free(picture);
    free(default_path);

    memset(&g_table, 0, sizeof(g_table));
    memset(g_widgets, 0, sizeof(g_widgets));
    build_table();
    area_layout(&g_table);
    area_draw(&g_table, true);

    widget = add_widget(TABLE_WIDGETS, "League Table", 7, 0, 0, 0);
    place(widget, 140, 60, 660, 115);
    widget_draw(widget, true);

    widget = add_widget(TABLE_WIDGETS + 1, "Continue", 4,
                        GUI_PANEL_FILL | GUI_PANEL_BORDER, GUI_COLOUR_GREY, 0);
    place(widget, 500, 495, 660, 535);
    widget_draw(widget, true);

    widget = add_widget(TABLE_WIDGETS + 2, "Form", 4,
                        GUI_PANEL_FILL | GUI_PANEL_BORDER, GUI_COLOUR_GREY,
                        GUI_TEXT_LEFT);
    widget->flags = WIDGET_MARK_UP;
    place(widget, 320, 495, 480, 535);
    widget_draw(widget, true);

    widget = add_widget(TABLE_WIDGETS + 3, "Use the up and down keys", 2, 0, 0,
                        GUI_TEXT_LEFT);
    place(widget, 140, 495, 310, 535);
    widget_draw(widget, true);
}

void demo_scroll(s32 rows)
{
    s32 row = g_table.scrollbar.position + rows;

    if (row < 0) row = 0;
    if (row > g_table.scrollbar.max_position) row = g_table.scrollbar.max_position;
    g_table.scroll_to = row;
    area_release(&g_table);
    area_layout(&g_table);
    area_draw(&g_table, true);
}
