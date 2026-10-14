#include "demo.h"
#include "gui_text.h"
#include "gui_utils.h"
#include "scrman.h"
#include "screen_stack.h"
#include <stdio.h>

#define TABLE_COLUMNS 4
#define TABLE_ROWS    10
#define TEAM_COUNT    16

#define COLOUR_WHITE     0xffff
#define COLOUR_DARK_BLUE 0x0129

static const char *const g_teams[TEAM_COUNT] = {
    "Arsenal", "Aston Villa", "Blackburn Rovers", "Bolton Wanderers",
    "Charlton Athletic", "Chelsea", "Derby County", "Everton", "Fulham",
    "Ipswich Town", "Leeds United", "Leicester City", "Liverpool",
    "Manchester United", "Middlesbrough", "Newcastle United",
};

static scrman_t g_screen;

static void add_widget(u32 flags, s32 x1, s32 y1, s32 x2, s32 y2, s32 column,
                       s32 row, u32 style, u16 colour, u32 text_flags,
                       s16 font, const char *text, s16 area)
{
    scrman_widget_create(&g_screen, flags, x1, y1, x2, y2, column, row, style,
                         colour, GUI_COLOUR_YELLOW, text_flags, font,
                         style ? GUI_COLOUR_BLACK : COLOUR_WHITE, text, 0, 0, 0,
                         area);
}

static void build_table(void)
{
    static const u8 weights[TABLE_COLUMNS] = { 6, 1, 1, 1 };
    s16 table = scrman_area_create(&g_screen, 140, 130, 660, 470,
                                   TABLE_COLUMNS, weights, TABLE_ROWS, NULL,
                                   GUI_PANEL_FILL | GUI_PANEL_BORDER,
                                   COLOUR_DARK_BLUE, -1);

    for (s32 team = 0; team < TEAM_COUNT; team++) {
        s32 points = 3 * (TEAM_COUNT - team) + team % 3;

        for (s32 column = 0; column < TABLE_COLUMNS; column++) {
            char number[16];

            snprintf(number, sizeof(number), "%d",
                     column == 1 ? 38 : column == 2 ? points / 3 : points);
            add_widget(0, 0, 0, 0, 0, column, team, 0, 0,
                       column == 0 ? GUI_TEXT_LEFT : 0, 3,
                       column == 0 ? g_teams[team] : number, table);
        }
    }
}

void demo_draw(const char *background)
{
    scrman_init(&g_screen, 0);
    scrman_reset(&g_screen, true);
    build_table();
    add_widget(0, 140, 60, 660, 115, 0, 0, 0, 0, 0, 7, "League Table", -1);
    add_widget(0, 500, 495, 660, 535, 0, 0, GUI_PANEL_FILL | GUI_PANEL_BORDER,
               GUI_COLOUR_GREY, 0, 4, "Continue", -1);
    add_widget(WIDGET_MARK_UP, 320, 495, 480, 535, 0, 0,
               GUI_PANEL_FILL | GUI_PANEL_BORDER, GUI_COLOUR_GREY,
               GUI_TEXT_LEFT, 4, "Form", -1);
    add_widget(0, 140, 495, 310, 535, 0, 0, 0, 0, GUI_TEXT_LEFT, 2,
               "Use the up and down keys", -1);
    /* A backslash names no file, so the screen draw shows DEFAULT_PIC.RGN. */
    static char name[SCREEN_TITLE_SIZE];

    snprintf(name, sizeof(name), "%s", background ? background : "\\");
    scrman_draw(&g_screen, name, -1, -1, -1, -1);
}

void demo_scroll(s32 rows)
{
    area_t *table = &g_screen.areas[0];
    s32 row = table->scrollbar.position + rows;

    if (row < 0) row = 0;
    if (row > table->scrollbar.max_position) row = table->scrollbar.max_position;
    table->scroll_to = row;
    area_release(table);
    area_layout(table);
    area_draw(table, true);
}
