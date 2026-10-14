#include "file_screens.h"
#include "background.h"
#include "gui_menu.h"
#include "gui_utils.h"
#include "langlib.h"
#include "menubar.h"
#include "screen_stack.h"
#include <stddef.h>

s32 g_dialog_code;
s32 g_dialog_button;

/* A colour in a slot, sign-extended to 32 bits as the original keeps it. */
static uintptr_t colour_slot(u16 colour)
{
    return (uintptr_t)(u32)(s32)(s16)colour;
}

void confirm_push(const char *title, const char *heading, const char *text,
                  s32 buttons, u16 text_colour, u16 colour, s32 code)
{
    screen_stack_t *st = &g_screen_stack;

    /* The original shows an error box here (file_screens.cpp 1506). */
    if (!title || !heading || !text) return;
    if (!screen_stack_push(st, confirm_build, confirm_input, 1, NULL, NULL)) {
        return;
    }
    screen_stack_set_slot(st, 0, (uintptr_t)title, true);
    screen_stack_set_slot(st, 1, (uintptr_t)heading, true);
    screen_stack_set_slot(st, 2, (uintptr_t)text, true);
    screen_stack_set_slot(st, 3, (uintptr_t)(u32)buttons, false);
    screen_stack_set_slot(st, 4, colour_slot(text_colour), false);
    screen_stack_set_slot(st, 5, colour_slot(colour), false);
    screen_stack_set_slot(st, 6, 0, false);
    screen_stack_set_slot(st, 7, (uintptr_t)(u32)code, false);
}

void confirm_build(void)
{
    static const u8 weights[2] = {3, 1};
    screen_stack_t *st = &g_screen_stack;
    scrman_t *sm = st->record.screen_manager;
    const char *title = (const char *)screen_stack_slot(st, 0);
    const char *heading = (const char *)screen_stack_slot(st, 1);
    const char *text = (const char *)screen_stack_slot(st, 2);
    s32 buttons = (s32)screen_stack_slot(st, 3);
    u16 text_colour = (u16)screen_stack_slot(st, 4);
    u16 colour = (u16)screen_stack_slot(st, 5);
    s16 area;

    pick_background();
    menubar_build(4, 0);
    screen_heading(st, colour, text_colour, title);
    screen_sub_heading(st, heading);
    /*
     * Without a text the original lists the lines of the object in slot 6
     * (FUN_005ed200). Only FUN_005919b0 pushes such an object, and no ported
     * screen calls it.
     */
    if (text) {
        gui_text_box(110, 145, 780, 535, 2, 0, 16, 4, GUI_COLOUR_LIGHT_GREY,
                     text);
    }
    if (buttons == 0) {
        text_format(g_text_buffer, "Ok");
        scrman_widget_create(sm, 2, 100, 555, 790, 590, 0, 0, 0x30,
                             GUI_COLOUR_GREY, GUI_COLOUR_GREY, 0xc, 4,
                             GUI_COLOUR_LIGHT_GREY, g_text_buffer, 0, 3, 0,
                             -1);
        return;
    }
    if (buttons == 1) {
        area = scrman_area_create(sm, 100, 555, 790, 590, 2, weights, 1, NULL,
                                  1, 0, -1);
    } else {
        area = scrman_area_create(sm, 0, 0, 3, 1, 2, weights, 1, NULL, 1, 0,
                                  -1);
    }
    text_format(g_text_buffer, "No");
    screen_cell_button(st, area, 0, 0, 0x30, GUI_COLOUR_GREY, 0xc, 3,
                       GUI_COLOUR_LIGHT_GREY, g_text_buffer, 1, 0);
    text_format(g_text_buffer, "Yes");
    screen_cell_button(st, area, 1, 0, 0x30, GUI_COLOUR_GREY, 0xc, 3,
                       GUI_COLOUR_LIGHT_GREY, g_text_buffer, 2, 0);
}

s32 confirm_input(s32 clicked)
{
    screen_stack_t *st = &g_screen_stack;
    s16 id = screen_stack_clicked_id(st, (s16)clicked);
    s32 result;

    if (id >= 1 && id <= 5) {
        /* Slot 6 holds an object only after FUN_005919b0, which frees it here. */
        g_dialog_code = (s32)screen_stack_slot(st, 7);
        g_dialog_button = id;
        return SCREEN_POP;
    }
    result = menubar_input(clicked);
    if (result == 0) result = menubar_input_more(clicked);
    return result;
}
