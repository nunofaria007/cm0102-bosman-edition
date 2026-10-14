#include "menubar.h"
#include "file_screens.h"
#include "gui_utils.h"
#include "langlib.h"
#include "screen_stack.h"
#include "setup.h"
#include "utils.h"
#include <time.h>

/* Dialog codes of the "Please Confirm" screens of the side bar. */
#define CODE_EXIT    0xb
#define CODE_RESTART 0x3d

#define ID_BACK    -2
#define ID_FORWARD -3
#define ID_EXIT    0x402
#define ID_RESTART 0x42f

s32 g_menubar_time;

void menubar_build(u8 mode, s32 arg2)
{
    screen_stack_t *st = &g_screen_stack;
    scrman_t *sm = st->record.screen_manager;
    s32 flagged = screen_stack_flagged(st);
    s32 row = 0;
    s16 bar, cells, arrows;

    (void)arg2;
    g_menubar_time = (s32)time(NULL);
    /* FUN_00823930 runs on a click with id 0x423. It is not ported. */
    st->hook = NULL;
    st->hook_id = 0x423;
    st->unknown_37f6 = 0x424;
    /* With no manager record the original always takes mode 1. */
    mode = 1;
    (void)mode;

    bar = scrman_area_create(sm, 0, 0, 89, 599, 1, NULL, 1, NULL, 1, 0, -1);
    scrman_widget_create(sm, 0x400, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0xc, 1, 0,
                         utils_path("CM3_DATA", "game.mbr"), 0, 0, 0, bar);
    cells = scrman_area_create(sm, 5, 10, 85, 590, 1, NULL, 13, NULL, 1, 0,
                               -1);

    text_format(g_text_buffer, "Version\n<%s - version>", langlib_version());
    screen_cell_label(st, cells, 0, row, 0x1061, 0, 0xc, 1, GUI_COLOUR_YELLOW,
                      g_text_buffer);

    /*
     * With a font setting other than -1 the original shows the characters
     * 0x28 and 0x29 in font 0 instead. The port always has -1.
     */
    row++;
    arrows = scrman_area_create(sm, 5, 10, 85, 590, 2, NULL, 13, NULL, 1, 0,
                                -1);
    screen_cell_button(st, arrows, 0, row, 0x1021, 0, 0xc, 1,
                       GUI_COLOUR_LIGHT_GREY, "<<<", ID_BACK, 0);
    screen_cell_button(st, arrows, 1, row, 0x1021, 0, 0xc, 1,
                       GUI_COLOUR_LIGHT_GREY, ">>>", ID_FORWARD, 0);

    row++;
    text_format(g_text_buffer, "Add\nManager");
    screen_cell_label(st, cells, 0, row, 0x1021, 0, 0x2c, 1,
                      GUI_COLOUR_LIGHT_GREY, g_text_buffer);

    row++;
    text_format(g_text_buffer, "Restart\nGame");
    if (st->record.player == 0 && flagged == 0) {
        screen_cell_button(st, cells, 0, row, 0x1021, 0, 0xc, 1,
                           GUI_COLOUR_YELLOW, g_text_buffer, ID_RESTART, 0);
    } else {
        screen_cell_label(st, cells, 0, row, 0x1021, 0, 0x2c, 1,
                          GUI_COLOUR_YELLOW, g_text_buffer);
    }

    row++;
    text_format(g_text_buffer, "Exit\nGame");
    if (flagged == 0) {
        screen_cell_button(st, cells, 0, row, 0x1021, 0, 0xc, 1,
                           GUI_COLOUR_YELLOW, g_text_buffer, ID_EXIT, 0);
    } else {
        screen_cell_label(st, cells, 0, row, 0x1021, 0, 0x2c, 1,
                          GUI_COLOUR_YELLOW, g_text_buffer);
    }
}

/* The "Please Confirm" screen of "Restart Game" and "Exit Game". */
static void confirm(const char *question, s32 code)
{
    char heading[0x100];

    text_format(g_text_buffer, question);
    confirm_push(setup_title(), text_format(heading, "Please Confirm"),
                 g_text_buffer, 1, GUI_COLOUR_LIGHT_GREY, GUI_COLOUR_RED,
                 code);
}

/*
 * The answers of the "Please Confirm" screens, after the main loop pops
 * them (0x76239a). The codes 1, 7, 8, 9, 0xa, 0x2a, 0x37, 0x3a, 0x3e, 0x43
 * and 0x44 come from screens that the port does not have. Returns a result
 * for the main loop, or 0 to go on.
 */
static s32 answer(void)
{
    screen_stack_t *st = &g_screen_stack;
    s32 button = g_dialog_button;

    if (g_dialog_code == CODE_RESTART) {
        g_dialog_code = 0;
        g_dialog_button = 0;
        /*
         * With a flagged screen open the original shows another dialog
         * (0xa5a9b8), and with a manager record it saves first. Neither can
         * happen on the start screen, so neither is ported.
         */
        if (button == 2 && screen_stack_flagged(st) == 0) {
            /* The original starts a new copy of itself and ends (0x68dbc0). */
            screen_stack_free_all(st);
            setup_push_start_screen();
            return SCREEN_REBUILD;
        }
        button = 0;
    }
    if (g_dialog_code == CODE_EXIT) {
        if (st->injected_id == ID_EXIT) st->injected_id = 0;
        g_dialog_code = 0;
        g_dialog_button = 0;
        if (button == 2) {
            if (st->record.player != 0) return SCREEN_QUIT_12;
            /*
             * With a flagged screen open the original shows another dialog
             * (0xa5a944), which the start screen cannot reach.
             */
            if (screen_stack_flagged(st) == 0) {
                /* The original shuts down and calls exit(0) here (0x762c4d). */
                return SCREEN_QUIT;
            }
        }
    }
    return 0;
}

s32 menubar_input(s32 clicked)
{
    screen_stack_t *st = &g_screen_stack;
    s16 id = screen_stack_clicked_id(st, (s16)clicked);

    if (id == 0x424) return SCREEN_REBUILD_11;
    /* FUN_00725ca0 acts on the codes 1 and 0x43 and the id 0x3e8 only. */
    if (st->after_pop) {
        s32 result = answer();

        if (result) return result;
    }
    id = screen_stack_clicked_id(st, (s16)clicked);
    if (id == ID_BACK && screen_stack_cannot_go_back(st, 0) == 0) {
        return SCREEN_BACK;
    }
    if (id == ID_FORWARD && screen_stack_at_end(st) == 0) {
        return SCREEN_FORWARD;
    }
    /* The ids from 0x3e8 to 0x434 between here need a game. */
    if (id == ID_RESTART) {
        confirm("Restart the game ?", CODE_RESTART);
        return SCREEN_REBUILD;
    }
    if (id == ID_EXIT && screen_stack_flagged(st) == 0) {
        confirm(st->record.player == 0 ? "Exit the game ?"
                                       : "Exit the network game ?",
                CODE_EXIT);
        return SCREEN_REBUILD;
    }
    return 0;
}

s32 menubar_input_more(s32 clicked)
{
    (void)clicked;
    return 0;
}
