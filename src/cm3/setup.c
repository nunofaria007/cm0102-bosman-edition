#include "setup.h"
#include "area.h"
#include "background.h"
#include "database.h"
#include "gui_menu.h"
#include "gui_utils.h"
#include "key_index.h"
#include "key_nation.h"
#include "langlib.h"
#include "menubar.h"
#include "screen_stack.h"
#include <stdio.h>
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
    text_format(g_title_buffer, sizeof(g_title_buffer), "Championship Manager 2001/02");
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
    text_format(g_title_buffer, sizeof(g_title_buffer), "Championship Manager 2001/02");
    screen_heading(st, GUI_COLOUR_RED, GUI_COLOUR_LIGHT_GREY, g_title_buffer);
    menubar_build(4, 0);
    text_format(g_text_buffer, sizeof(g_text_buffer), "Setup Game");
    screen_sub_heading(st, g_text_buffer);
    /*
     * "Network Play" has id 0 and grey text when the command line has
     * -nonetwork (DAT_00a801c4 is 0). The port has no such option.
     */
    for (s32 i = 0; i < MENU_ITEMS; i++) {
        /* The original passes a fixed text as the format, so its '<' notes work. */
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wformat-security"
        text_format(items[i].text, sizeof(items[i].text), g_menu_texts[i]);
#pragma clang diagnostic pop
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
    /* FUN_0081ce00. The original ends the program when the load fails. */
    if (id == 1) return setup_start_new_game() ? SCREEN_REBUILD : SCREEN_QUIT;
    /* 2 to 9: the actions of the other menu items are not ported. */
    if (id >= 2 && id <= MENU_ITEMS) return SCREEN_REBUILD;
    if (id == -3) return 0;
    result = menubar_input(clicked);
    if (result == 0) result = menubar_input_more(clicked);
    return result;
}

s32 g_leagues_to_load;
s32 g_selected_nations;
struct nation *g_first_selected;

/* FUN_00827d50. */
bool setup_default_leagues(void)
{
    /* The nation of each language index, from the jump table at 0x8280dc. */
    static const s32 nation_of_language[13] = {
        KEY_ENGLAND, KEY_ENGLAND, KEY_FRANCE, KEY_GERMANY, KEY_ITALY,
        KEY_SPAIN, KEY_DENMARK, KEY_HOLLAND, KEY_NORWAY, KEY_PORTUGAL,
        KEY_SWEDEN, KEY_JAPAN, KEY_SOUTH_KOREA,
    };
    nation_t *n;

    /* 0x827d60: "Setup.cpp" line 0x1d88 error box when no nation table. */
    if (!g_nations) return false;
    for (s32 i = 0; i < g_nation_count; i++) g_nations[i].league_flags = 0;
    g_leagues_to_load = 0;
    g_selected_nations = 0;
    g_first_selected = NULL;
    /* 0x828075: "Setup.cpp" line 0x1e09 error box for another index. */
    if ((u32)g_language_index > 12) return false;
    /*
     * A key id of -1 makes the original write before the start of the
     * table. The port selects nothing then.
     */
    n = key_nation(nation_of_language[g_language_index]);
    if (!n) return false;
    n->league_flags = 2;
    g_leagues_to_load = 1;
    g_selected_nations = 1;
    g_first_selected = n;
    return true;
}


u8 g_real_players = 1;
u8 g_attribute_masking = 1;

/* Ids of "Select League(s)". */
#define ID_SELECT_ALL     0xa
#define ID_DESELECT_ALL   0xb
#define ID_NAME           0xc
#define ID_SELECTED       0xd
#define ID_BACKGROUND     0xe
#define ID_EXTRA_DIVISION 0x38
#define ID_REAL_YES       0x39
#define ID_REAL_NO        0x3a
#define ID_MASKING_YES    0x3b
#define ID_MASKING_NO     0x3c

/* League flags of a nation (nation +0x11c). */
#define LEAGUE_BACKGROUND 1
#define LEAGUE_SELECTED   2
#define LEAGUE_EXTRA      4

/* FUN_0081ce00. */
bool setup_start_new_game(void)
{
    screen_stack_t *st = &g_screen_stack;

    if (!g_nations) {
        /*
         * The original checks the free memory first (FUN_007a1710) and
         * pushes the "No Memory" screen under 50 MB. The port does not.
         */
        if (!database_load(g_data_dir)) {
            /* "Setup.cpp" line 0x688 error box, then exit(-1). */
            fprintf(stderr, "cm0102: the database in %s does not load\n", g_data_dir);
            return false;
        }
        /* FUN_005d8480, the settings apply, is not ported. */
        setup_default_leagues();
        if (!league_table_build()) {
            /* "Setup.cpp" line 0x693 error box, then exit(-1). */
            fprintf(stderr, "cm0102: no league table\n");
            return false;
        }
    }
    if (screen_stack_push(st, select_leagues_build, select_leagues_input, 0,
                          NULL, NULL)) {
        screen_stack_set_slot(st, 0, 0, false);   /* scroll position */
        screen_stack_set_slot(st, 1, 0, false);   /* list area */
    }
    return true;
}

/* A "Yes" or "No" button of the options: orange and outlined when chosen. */
static void option_button(screen_stack_t *st, s16 area, s32 column,
                          const char *text, s32 id, bool chosen)
{
    text_format(g_text_buffer, sizeof(g_text_buffer), "%s", text);
    screen_cell_button(st, area, column, 0, chosen ? 0x801 : 1,
                       GUI_COLOUR_GREY, 0xc, 1,
                       chosen ? GUI_COLOUR_ORANGE : GUI_COLOUR_GREY,
                       g_text_buffer, id, 0);
}

/* "SELECTED", "BACKGROUND" or the extra division of a row of the list. */
static void flag_button(screen_stack_t *st, s16 area, s32 column, s32 row,
                        s32 id, nation_t *n, bool set)
{
    screen_cell_button(st, area, column, row, set ? 0x801 : 1, 0, 0xc, 2,
                       set ? GUI_COLOUR_ORANGE : GUI_COLOUR_GREY,
                       g_text_buffer, id, (intptr_t)n);
}

/* Build 0x81d010, the branches for DAT_00dd7118 = 0. */
void select_leagues_build(void)
{
    static const u8 list_weights[7] = {25, 1, 13, 1, 15, 1, 19};
    static const u8 option_weights[9] = {42, 13, 1, 13, 10, 42, 13, 1, 13};
    static const u8 masking_weights[9] = {0, 0, 0, 0, 0, 42, 13, 1, 13};
    screen_stack_t *st = &g_screen_stack;
    scrman_t *sm = st->record.screen_manager;
    const u8 *order;
    s16 area, list;
    s32 row = 0;

    /*
     * The original also sets DAT_00dd7124 to 0 and takes the free memory
     * in MB / 7, at least 1, for the "Select All" limit. The port acts as
     * if memory is enough.
     */
    pick_background();
    text_format(g_title_buffer, sizeof(g_title_buffer), "Championship Manager 2001/02");
    screen_heading(st, GUI_COLOUR_RED, GUI_COLOUR_LIGHT_GREY, g_title_buffer);
    menubar_build(4, 0);
    text_format(g_text_buffer, sizeof(g_text_buffer), "Select League(s)");
    screen_sub_heading(st, g_text_buffer);

    /* The options. DAT_00b74348 is 0 on a plain start. */
    area = scrman_area_create(sm, 110, 145, 525, 165, 9, option_weights, 1,
                              NULL, 2, 0, -1);
    text_format(g_text_buffer, sizeof(g_text_buffer), "  Use Real Players:");
    screen_cell_button(st, area, 0, 0, 1, GUI_COLOUR_GREY, 1, 1,
                       GUI_COLOUR_LIGHT_GREY, g_text_buffer, 0, 0);
    option_button(st, area, 1, "Yes", ID_REAL_YES, g_real_players != 0);
    option_button(st, area, 3, "No", ID_REAL_NO, g_real_players == 0);
    /* 0x81d33e: an options area with index 0 gets a second area. */
    if (area == 0) {
        area = scrman_area_create(sm, 110, 145, 525, 165, 9, masking_weights,
                                  1, NULL, 2, 0, -1);
    }
    text_format(g_text_buffer, sizeof(g_text_buffer), "  Attribute Masking:");
    screen_cell_button(st, area, 5, 0, 1, GUI_COLOUR_GREY, 1, 1,
                       GUI_COLOUR_LIGHT_GREY, g_text_buffer, 0, 0);
    option_button(st, area, 6, "Yes", ID_MASKING_YES, g_attribute_masking != 0);
    option_button(st, area, 8, "No", ID_MASKING_NO, g_attribute_masking == 0);

    /* "Select All" and "De-Select All", labels when they would do nothing. */
    area = scrman_area_create(sm, 530, 145, 780, 165, 2, NULL, 1, NULL, 1, 0, -1);
    text_format(g_text_buffer, sizeof(g_text_buffer), "Select All");
    if (g_selected_nations < 15) {
        screen_cell_button(st, area, 0, 0, 0x30, GUI_COLOUR_GREY, 0xc, 1,
                           GUI_COLOUR_LIGHT_GREY, g_text_buffer, ID_SELECT_ALL, 0);
    } else {
        screen_cell_label(st, area, 0, 0, 0x30, GUI_COLOUR_GREY, 0x2c, 1,
                          GUI_COLOUR_LIGHT_GREY, g_text_buffer);
    }
    text_format(g_text_buffer, sizeof(g_text_buffer), "De-Select All");
    if (g_leagues_to_load > 0) {
        screen_cell_button(st, area, 1, 0, 0x30, GUI_COLOUR_GREY, 0xc, 1,
                           GUI_COLOUR_LIGHT_GREY, g_text_buffer, ID_DESELECT_ALL, 0);
    } else {
        screen_cell_label(st, area, 1, 0, 0x30, GUI_COLOUR_GREY, 0x2c, 1,
                          GUI_COLOUR_LIGHT_GREY, g_text_buffer);
    }

    /* The list, 16 rows in view. */
    list = scrman_area_create(sm, 110, 170, 780, 535, 7, list_weights, 16,
                              NULL, 2, 0, -1);
    order = league_order(0);
    g_selected_nations = 0;
    for (s32 i = 0; i < LEAGUE_COUNT; i++) {
        const nation_t *n = g_leagues[order[i]].nation;

        if (n && (n->league_flags & LEAGUE_SELECTED)) g_selected_nations++;
    }
    for (s32 i = 0; i < LEAGUE_COUNT; i++) {
        const league_t *league = &g_leagues[order[i]];
        nation_t *n = league->nation;

        if (!n) continue;
        snprintf(g_text_buffer, sizeof(g_text_buffer), "  %s", n->name);
        if (!league->setup) {
            screen_cell_label(st, list, 0, row, 1, 0, 1, 3, GUI_COLOUR_GREY,
                              g_text_buffer);
            row++;
            continue;
        }
        screen_cell_button(st, list, 0, row, 1, 0, 1, 3,
                           n->league_flags ? GUI_COLOUR_YELLOW : GUI_COLOUR_LIGHT_GREY,
                           g_text_buffer, ID_NAME, (intptr_t)n);
        text_format(g_text_buffer, sizeof(g_text_buffer), "SELECTED");
        flag_button(st, list, 2, row, ID_SELECTED, n,
                    n->league_flags & LEAGUE_SELECTED);
        text_format(g_text_buffer, sizeof(g_text_buffer), "BACKGROUND");
        flag_button(st, list, 4, row, ID_BACKGROUND, n,
                    n->league_flags & LEAGUE_BACKGROUND);
        if (league_extra_division(n, g_text_buffer, sizeof(g_text_buffer))) {
            flag_button(st, list, 6, row, ID_EXTRA_DIVISION, n,
                        n->league_flags & LEAGUE_EXTRA);
        }
        row++;
    }

    gui_back_next_bar(screen_stack_cannot_go_back(st, 0) == 0,
                      g_selected_nations != 0);
    if (list != -1) {
        area_set_scroll_to(&sm->areas[list], (s32)screen_stack_slot(st, 0));
    }
    screen_stack_set_slot(st, 1, (uintptr_t)(u32)(s32)list, false);
}

/*
 * The new first selected nation, in table order: any selected nation, or
 * with `exact` a nation that is only selected (0x81e0c2, 0x81e62a).
 */
static nation_t *first_selected(bool exact)
{
    for (s32 i = 0; i < LEAGUE_COUNT; i++) {
        nation_t *n = g_leagues[i].nation;

        if (!n) continue;
        if (exact ? n->league_flags == LEAGUE_SELECTED
                  : (n->league_flags & LEAGUE_SELECTED) != 0) {
            return n;
        }
    }
    return NULL;
}

/*
 * A nation with no flags becomes a selected league. The original checks
 * the memory first (0x81dfc2): with as many leagues as the free memory in
 * MB / 7, DAT_009cd1fc set and no "-dontcheckram", it shows "Not enough
 * memory" and changes nothing. The port does not.
 */
static void select_nation(nation_t *n)
{
    g_leagues_to_load++;
    g_selected_nations++;
    n->league_flags = LEAGUE_SELECTED;
    if (!g_first_selected) g_first_selected = n;
}

/* Input 0x81dc30, the branches for DAT_00dd7118 = 0. */
s32 select_leagues_input(s32 clicked)
{
    screen_stack_t *st = &g_screen_stack;
    scrman_t *sm = st->record.screen_manager;
    nation_t *n = NULL;
    s32 result;
    s16 id;

    if (!g_nations) return SCREEN_BACK;
    if ((s16)clicked != -1) {
        /* The first row in view, for the next build. */
        s32 shown = screen_stack_shown_row(st, (s16)screen_stack_slot(st, 1));

        screen_stack_set_slot(st, 0, (uintptr_t)(u32)shown, false);
        n = (nation_t *)sm->widgets[(s16)clicked].unknown_48;
    }
    id = screen_stack_clicked_id(st, (s16)clicked);
    /* The original reads a NULL nation for an injected id. The port stops. */
    if (!n && (id == ID_NAME || id == ID_SELECTED || id == ID_BACKGROUND ||
               id == ID_EXTRA_DIVISION)) {
        return SCREEN_REBUILD_11;
    }
    switch (id) {
    case ID_SELECT_ALL:
        for (s32 i = 0; i < LEAGUE_COUNT; i++) {
            nation_t *m = g_leagues[i].nation;

            if (!m || !g_leagues[i].setup) continue;
            if (m->league_flags == 0) {
                g_leagues_to_load++;
                g_selected_nations++;
                m->league_flags = LEAGUE_SELECTED;
            } else if (m->league_flags & LEAGUE_BACKGROUND) {
                g_selected_nations++;
                m->league_flags = (u8)((m->league_flags - 1) | LEAGUE_SELECTED);
            } else {
                continue;
            }
            if (!g_first_selected) g_first_selected = m;
        }
        return SCREEN_REBUILD_11;
    case ID_REAL_YES:
        g_real_players = 1;
        return SCREEN_REBUILD_11;
    case ID_REAL_NO:
        g_real_players = 0;
        return SCREEN_REBUILD_11;
    case ID_MASKING_NO:
        g_attribute_masking = 0;
        return SCREEN_REBUILD_11;
    case ID_MASKING_YES:
        g_attribute_masking = 1;
        return SCREEN_REBUILD_11;
    case ID_DESELECT_ALL:
        for (s32 i = 0; i < g_nation_count; i++) g_nations[i].league_flags = 0;
        g_leagues_to_load = 0;
        g_selected_nations = 0;
        g_first_selected = NULL;
        return SCREEN_REBUILD_11;
    case ID_NAME:
        if (n->league_flags == 0) {
            select_nation(n);
            return SCREEN_REBUILD_11;
        }
        g_leagues_to_load--;
        if (n->league_flags & LEAGUE_SELECTED) g_selected_nations--;
        n->league_flags = 0;
        if (n == g_first_selected) g_first_selected = first_selected(false);
        return SCREEN_REBUILD_11;
    case ID_EXTRA_DIVISION:
        if (n->league_flags == 0) select_nation(n);
        if (n->league_flags & LEAGUE_EXTRA) {
            n->league_flags -= LEAGUE_EXTRA;
            g_leagues_to_load--;
        } else {
            n->league_flags |= LEAGUE_EXTRA;
            g_leagues_to_load++;
        }
        return SCREEN_REBUILD_11;
    case ID_SELECTED:
        if (n->league_flags == 0) {
            select_nation(n);
        } else if (n->league_flags & LEAGUE_SELECTED) {
            g_leagues_to_load--;
            if (n->league_flags & LEAGUE_EXTRA) g_leagues_to_load--;
            g_selected_nations--;
            n->league_flags = 0;
            if (n == g_first_selected) g_first_selected = first_selected(false);
        } else {
            g_selected_nations++;
            n->league_flags = (u8)((n->league_flags & ~LEAGUE_BACKGROUND) | LEAGUE_SELECTED);
            if (!g_first_selected) g_first_selected = n;
        }
        return SCREEN_REBUILD_11;
    case ID_BACKGROUND:
        if (n->league_flags == 0) {
            /* After the same memory check as select_nation. */
            g_leagues_to_load++;
            n->league_flags = LEAGUE_BACKGROUND;
        } else if (n->league_flags & LEAGUE_BACKGROUND) {
            g_leagues_to_load--;
            if (n->league_flags & LEAGUE_EXTRA) g_leagues_to_load--;
            n->league_flags = 0;
        } else {
            g_selected_nations--;
            n->league_flags = (u8)((n->league_flags & ~LEAGUE_SELECTED) | LEAGUE_BACKGROUND);
            if (n == g_first_selected) g_first_selected = first_selected(true);
        }
        return SCREEN_REBUILD_11;
    case SCREEN_FORWARD:
        /*
         * "Next". With more than one nation the original pushes "Select
         * Start Season", with one it creates the game (FUN_00828110).
         * Neither is ported.
         */
        if (g_selected_nations == 0) return 0;
        return SCREEN_REBUILD;
    default:
        break;
    }
    result = menubar_input(clicked);
    if (result == 0) result = menubar_input_more(clicked);
    return result;
}
