#include "progress_box.h"
#include "display.h"
#include "gui_text.h"
#include "gui_utils.h"

#define BAR_X1    272
#define BAR_WIDTH 496

s32 gui_progress_box(const char *text, s32 step, s32 total)
{
    /* The original returns 0 here when its display is off (DAT_00aebe0c). */
    if (g_screen_pump) g_screen_pump();         /* FUN_0068d520 */
    if (!text) return 0;
    if (step == 0 && g_screen_stack.record.screen_manager) {
        scrman_darken_once(g_screen_stack.record.screen_manager);
    }
    gui_panel(100, 555, 790, 590, GUI_PANEL_FILL | GUI_PANEL_BORDER,
              GUI_COLOUR_GREY);
    if (total > 0 && step <= total) {
        /* The original multiplies in 32 bits, so a large step wraps. */
        s32 end = (s32)((u32)step * BAR_WIDTH) / total + BAR_X1;

        gui_text(120, 565, 250, 580, 0xc, 1, GUI_COLOUR_LIGHT_GREY, text, -1);
        gui_panel(270, 565, 770, 580, GUI_PANEL_BORDER | GUI_PANEL_SUNKEN,
                  GUI_COLOUR_GREY);
        gui_rect(BAR_X1, 567, end, 578, GUI_RECT_FILL, GUI_COLOUR_NAVY);
    } else {
        gui_text(120, 565, 770, 580, 0xc, 1, GUI_COLOUR_LIGHT_GREY, text, -1);
    }
    display_flip(100, 555, 790, 590);
    return 1;
}

/*
 * The original writes opcode 7, the step, the total and the text into its
 * packet buffer (S+0x37fe), then applies the packet on screen manager 0
 * (FUN_00564080, case 0x567013) or sends it to a remote player.
 */
void screen_stack_progress(screen_stack_t *st, const char *text, s32 step,
                           s32 total, s16 channel)
{
    if (!text) return;
    if (channel == -1) {
        /* The other 15 channels have no records in a local game. */
        if (st->record_count <= 0 || st->record.player != 0) return;
    } else if (channel == 0) {
        if (st->record.player != 0) return;
    } else if (channel < 1 || channel > 15) {
        return;
    }
    st->record.screen_manager->unknown_12f551 = 0;
    gui_progress_box(text, step, total);
}
