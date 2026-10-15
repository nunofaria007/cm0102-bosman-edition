/* The data set choice on the Setup Game screen (#39).
 *
 * The Setup Game screen (build 0x81bab0, input 0x81bd70) gets a tenth menu item, "Database". The build
 * fills 9 of the 25 menu items on its stack and calls the menu builder 0x5ed300 (0x81bcc5) with
 * (110, 145, 780, 535, 2 columns, 6 rows, font 3, count 9, items, 0, 0). The runtime fills item 9 and
 * passes count 10, so the button takes the free cell after "Web Sites" and the menu stays a full grid.
 * Item layout: +0x00 s16 font (-1 = default), +0x02 s16 text colour (0 = default), +0x04 text,
 * +0xcc id, +0xd0 data (docs/research/2026-10-13-screen-stack-inventory.md section 6a).
 *
 * A click on it moves the option database to the next data set and writes it to the settings file. The
 * input function then returns -4, so the screen builds again with the new text. The next Start New Game
 * loads the database from the chosen folder (files.c).
 *
 * The button only changes which files the next database load reads. It changes no game state, and a
 * saved game loads as before, because a save holds its own database. */
#include "win.h"
#include "options.h"

#include <string.h>

extern const uint32_t rt_site_setup_menu;

#define WIDGETS 0xb77c94u + 0xba95eu   /* the widgets of screen manager 0 (names.c) */
#define WIDGET_SIZE 0x18cu
#define MAX_WIDGETS 1200
#define ITEM_SIZE 0xd4u
#define ITEM_TEXT_SIZE 0xc8u
#define DATABASE_ID 0x7d39       /* the action id of the item; the original uses no such id */

static const struct { const char *value, *text; } sets[] = {
    {"3.9.60", "Database: 2001/02 (3.9.60)"},
    {"3.9.65", "Database: 2001/02 Dec (3.9.65)"},
    {"3.9.68", "Database: 2002/03 (3.9.68)"},
};
enum { SET_COUNT = 3 };

static int current_set(void)
{
    for (int i = 0; i < SET_COUNT; i++)
        if (!strcmp(options_string(OPT_DATABASE), sets[i].value)) return i;
    return SET_COUNT - 1;
}

void o_005ed300(void);
void o_0081bd70(void);

/* The menu builder, cdecl (x1, y1, x2, y2, columns, rows, font, count, items, highlight bits, sort). */
RT_WRAP(005ed300, menu_build)
{
    if (rd32(g_mem, cpu.esp) == rt_site_setup_menu && rt_arg(7) == 9) {
        uint32_t item = rt_arg(8) + 9 * ITEM_SIZE;
        wr16(g_mem, item, 0xffff);
        wr16(g_mem, item + 2, 0);
        win_put_str(item + 4, sets[current_set()].text, ITEM_TEXT_SIZE);
        wr32(g_mem, item + 0xcc, DATABASE_ID);
        wr32(g_mem, item + 0xd0, 0);
        wr32(g_mem, cpu.esp + 4 + 4 * 7, 10);
    }
    o_005ed300();
}

/* The Setup Game input, cdecl (clicked widget or -1). */
RT_WRAP(0081bd70, setup_input)
{
    int clicked = (int16_t)rt_arg(0);
    if (clicked >= 0 && clicked < MAX_WIDGETS &&
        (int16_t)rd16(g_mem, WIDGETS + WIDGET_SIZE * (uint32_t)clicked + 8) == DATABASE_ID) {
        options_set_enum(OPT_DATABASE, sets[(current_set() + 1) % SET_COUNT].value);
        win_data_set_changed();
        rt_ret_cdecl((uint32_t)-4);
        return;
    }
    o_0081bd70();
}
