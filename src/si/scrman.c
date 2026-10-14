#include "scrman.h"
#include "display.h"
#include "gui_utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>

extern void rgn_free(cm_image_t *img);
extern cm_image_t *rgn_load_into(const char *path, cm_image_t *reuse);
extern char *os_find_file_ci(const char *dir, const char *name);

s32 g_no_backgrounds;
s32 g_darken_screens;
const char *g_data_dir = "./Data";

/*
 * The original also writes each reset and each area create into a buffer for
 * network games (0xae2908). The port has no network games.
 */

void scrman_init(scrman_t *sm, u32 arg)
{
    sm->unknown_12f541 = arg;
    sm->background = NULL;
    sm->unknown_12f521 = NULL;
    sm->unknown_12f505 = 0;
    sm->unknown_12f555 = 0;
    sm->skip_draw = 0;
    sm->fade_in = 0;
    sm->laid_out = 0;
    sm->mode = 0;
    /* Differs from every file name, so the first draw loads its background. */
    sm->background_name[0] = 1;
    sm->unknown_12f529 = NULL;
    sm->area_slots = 0;
    sm->widget_slots = 0;
    sm->unknown_12f539 = 0;
    sm->unknown_12f53b = 0;
    sm->unknown_12f4ff = -1;
    sm->unknown_12f4fe = 0;
    sm->unknown_12f501 = -1;
    sm->unknown_12f503 = -1;
}

void scrman_reset(scrman_t *sm, bool release)
{
    sm->unknown_12f4ff = -1;
    sm->unknown_12f4fe = 0;
    if (sm->unknown_12f555 == 0 && release) {
        for (s16 i = 0; i < sm->area_slots; i++) {
            area_release(&sm->areas[i]);
        }
        for (s16 i = 0; i < sm->widget_slots; i++) {
            widget_release(&sm->widgets[i]);
        }
    }
    sm->mode = 0;
    sm->area_count = 0;
    sm->widget_count = 0;
    sm->widget_slots = 0;
    sm->area_slots = 0;
    sm->unknown_12f52d = -1;
    sm->unknown_12f52f = -1;
    sm->unknown_12f531 = -1;
    memset(sm->unknown_12f509, 0, sizeof(sm->unknown_12f509));
    sm->widget_overflow = 0;
    sm->area_overflow = 0;
    sm->unknown_12f533 = -1;
    sm->unknown_12f551 = 0;
    sm->unknown_12f555 = 0;
    sm->darken_screen = 0;
    rgn_free(sm->unknown_12f521);
    sm->unknown_12f521 = NULL;
    sm->laid_out = 0;
}

/* FUN_005603e0. */
static void append_to_order(scrman_t *sm, s16 index)
{
    if (index == -1) return;
    /* The original shows an error box here (display.cpp 770). */
    if (sm->area_count > SCRMAN_MAX_AREAS - 1) return;
    sm->area_order[sm->area_count++] = index;
}

s16 scrman_area_create(scrman_t *sm, s32 x1, s32 y1, s32 x2, s32 y2,
                       u8 column_count, const u8 *column_weights,
                       u8 row_count, const u8 *row_weights,
                       u32 flags, u16 colour, s16 owner_widget)
{
    s16 index = sm->area_slots;

    if (index >= SCRMAN_MAX_AREAS - 1) {
        sm->area_overflow = 1;
        return -1;
    }
    sm->area_slots++;
    memset(&sm->areas[index], 0, sizeof(sm->areas[index]));
    area_init(&sm->areas[index], sm->areas, sm->widgets, index, x1, y1, x2, y2,
              column_count, column_weights, row_count, row_weights,
              flags, colour, owner_widget);
    append_to_order(sm, index);
    return index;
}

s16 scrman_widget_create(scrman_t *sm, u32 flags, s32 x1, s32 y1, s32 x2,
                         s32 y2, s32 column, s32 row, u32 style, u16 colour,
                         u16 highlight_colour, u32 text_flags, s16 font,
                         u16 text_colour, const char *text, s32 unknown_40,
                         s32 unknown_08, intptr_t unknown_48, s16 area)
{
    s16 index = sm->widget_slots;

    if (index >= SCRMAN_MAX_WIDGETS - 1) {
        sm->widget_overflow = 1;
        return -1;
    }
    sm->widget_slots++;
    memset(&sm->widgets[index], 0, sizeof(sm->widgets[index]));
    widget_init(&sm->widgets[index], sm->areas, sm->widgets, index, flags,
                x1, y1, x2, y2, column, row, style, colour, highlight_colour,
                text_flags, font, text_colour, text, unknown_40, unknown_08,
                unknown_48, area);
    if (area != -1) {
        /* An index outside the areas reaches other memory in the original. */
        if (area >= 0 && area < SCRMAN_MAX_AREAS) {
            area_add_widget(&sm->areas[area], index);
        }
        return index;
    }
    /* The original shows an error box here (display.cpp 746). */
    if (sm->widget_count > SCRMAN_MAX_WIDGETS - 1) return index;
    sm->widget_order[sm->widget_count++] = index;
    return index;
}

/* Loads a new background when the name changes. */
static void update_background(scrman_t *sm, const char *name)
{
    cm_image_t *img;

    if (!strcasecmp(sm->background_name, name)) return;
    if (name[0] == '\0') {
        cm_image_t *capture = gui_capture(0, 0, 799, 599, sm->background);

        /* The original drops a background of another size without a free. */
        if (capture != sm->background) rgn_free(sm->background);
        sm->background = capture;
        sm->background_name[0] = '\0';
        return;
    }

    snprintf(sm->background_name, sizeof(sm->background_name), "%s", name);
    img = rgn_load_into(sm->background_name, sm->background);
    if (!img) {
        char *path = os_find_file_ci(g_data_dir, "DEFAULT_PIC.RGN");

        img = rgn_load_into(path, sm->background);
        free(path);
        /* The name stays, so the next draw does not try the file again. */
        if (!img) return;
    }
    if (img != sm->background) {
        rgn_free(sm->background);
        sm->background = img;
    }
}

void scrman_draw(scrman_t *sm, char *background, s16 x1, s16 y1, s16 x2,
                 s16 y2)
{
    if (sm->skip_draw) return;
    /* The original also clears the key that the input code queued. */
    if (g_no_backgrounds && background[0] != '\0') strcpy(background, "\\");
    update_background(sm, background);
    if (sm->background) gui_blit(0, 0, sm->background);

    if (!sm->laid_out) {
        for (s16 i = 0; i < sm->area_count; i++) {
            area_layout(&sm->areas[sm->area_order[i]]);
        }
    }
    for (s16 i = 0; i < sm->area_count; i++) {
        area_t *area = &sm->areas[sm->area_order[i]];

        if (area->shown && area->owner_widget == -1) area_draw(area, true);
    }
    for (s16 i = 0; i < sm->widget_count; i++) {
        widget_draw(&sm->widgets[sm->widget_order[i]], true);
    }
    for (s16 i = 0; i < sm->area_count; i++) {
        area_t *area = &sm->areas[sm->area_order[i]];

        if (area->shown && area->owner_widget != -1) area_draw(area, true);
    }

    if (sm->darken_screen && g_darken_screens) gui_darken(0, 0, 799, 599);
    if (sm->fade_in) {
        sm->fade_in = 0;
        gui_fade(0, 0, 799, 599, 20);
    }
    if (x1 == -1 || x2 == -1 || y1 == -1 || y2 == -1) {
        display_flip(0, 0, 799, 599);
    } else {
        display_flip(x1, y1, x2, y2);
    }
    sm->laid_out = 1;
}
