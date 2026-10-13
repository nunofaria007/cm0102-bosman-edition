#include "scrman.h"
#include <string.h>

extern void rgn_free(cm_image_t *img);

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
    sm->darken_once = 0;
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
    sm->clear_screen = 0;
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
