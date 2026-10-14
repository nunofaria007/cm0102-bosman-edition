#include "setup.h"
#include "background.h"
#include "gui_menu.h"
#include "gui_utils.h"
#include "langlib.h"
#include "menubar.h"
#include "screen_stack.h"
#include <string.h>

#define MENU_ITEMS 9

/* The menu of the start screen, with the ids that its input function reads. */
static const char *const g_menu_texts[MENU_ITEMS] = {
    "Start New Game", "Quick Start Game", "Restore Saved Game",
    "Delete Saved Game", "Network Play", "Game Settings", "Hall Of Fame",
    "Game Credits", "Web Sites",
};

void setup_push_start_screen(void)
{
    screen_stack_push(&g_screen_stack, setup_build, setup_input, 0, NULL,
                      NULL);
}

const char *setup_title(void)
{
    text_format(g_title_buffer, "Championship Manager 2001/02");
    return g_title_buffer;
}

void setup_build(void)
{
    screen_stack_t *st = &g_screen_stack;
    gui_menu_item_t items[25];

    /*
     * The original keeps the items on its stack, so their data (widget
     * +0x48) is whatever the stack held. The port sets it to 0.
     */
    memset(items, 0, sizeof(items));
    for (s32 i = 0; i < 25; i++) gui_menu_item_init(&items[i]);
    pick_background();
    text_format(g_title_buffer, "Championship Manager 2001/02");
    screen_heading(st, GUI_COLOUR_RED, GUI_COLOUR_LIGHT_GREY, g_title_buffer);
    menubar_build(4, 0);
    text_format(g_text_buffer, "Setup Game");
    screen_sub_heading(st, g_text_buffer);
    /*
     * "Network Play" has id 0 and grey text when the command line has
     * -nonetwork (DAT_00a801c4 is 0). The port has no such option.
     */
    for (s32 i = 0; i < MENU_ITEMS; i++) {
        text_format(items[i].text, g_menu_texts[i]);
        items[i].id = i + 1;
    }
    gui_menu_build(110, 145, 780, 535, 2, 6, 3, MENU_ITEMS, items, 0, 0);
    gui_back_next_bar(screen_stack_cannot_go_back(st, 0) == 0, 0);
    /* Slot 0 is set on a first run, which the port does not have. */
    if (screen_stack_slot(st, 0)) {
        scrman_area_create(st->record.screen_manager, 0, 0, 799, 599, 1, NULL,
                           1, NULL, 2, 0, -1);
    }
}

s32 setup_input(s32 clicked)
{
    screen_stack_t *st = &g_screen_stack;
    s16 id;
    s32 result;

    if (screen_stack_slot(st, 0)) {
        /* The original opens the "Game Settings" screen here (FUN_00823890). */
        screen_stack_set_slot(st, 0, 0, false);
        return SCREEN_REBUILD;
    }
    /*
     * Not ported: "-load" on the command line, the files quick_start.cfg and
     * auto_setup.cfg, and the answers of the load and quick start dialogs
     * (codes 4, 5 and 6 after a pop).
     */
    id = screen_stack_clicked_id(st, (s16)clicked);
    /* 1 to 9: the actions of the menu items are not ported. */
    if (id >= 1 && id <= MENU_ITEMS) return SCREEN_REBUILD;
    if (id == -3) return 0;
    result = menubar_input(clicked);
    if (result == 0) result = menubar_input_more(clicked);
    return result;
}
