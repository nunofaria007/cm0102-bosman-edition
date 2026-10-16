/* The Squad depth tab of the Team views screen (spec 2026-10-15-features-0.4.0, slice 5; Q106, Q107, Q156).
 * views.c builds the frame and calls squad_depth_build for tab 2.
 *
 * One row for each of the 11 slots of the tactic chosen in the Formation drop-down (the same button,
 * list and node slot 2 as Best XI), so DL and DR are apart and a tactic with two DC slots has two DC rows.
 * Each row holds the top three players for the slot from picker_depth (picker.c): by the picker's value,
 * best XI's player in the slot first of the available players. A player can appear in more than one row.
 * The players who cannot play the next match (injured, suspended, away with a national team, not
 * eligible) are in the rows too, with the name in grey. The number is the picker's score, 1 to 20, in the
 * attribute colour (Q156).
 *
 * For another club the picker counts only the attributes that the human manager's screens show
 * (0x543280), as in Best XI, so no hidden value feeds a number. The tab only reads the game (Q104). */
#include "win.h"
#include "views.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define GREY 0xae317eu              /* u16: the grey of the game's texts (columns.c) */
#define ITEM_HIGHLIGHT 0xaebdecu    /* u16: the colour of the list headings in Best XI */
#define LIST_TEXT 0xaebde8u         /* u16: the text colour of the squad list's cells (0x45c781) */
#define SLOTS 11
#define NAME_PX 150                 /* the widest name box, screen pixels: the text does not scale */

static int log_on(void)
{
    static int on = -1;
    if (on < 0) {
        const char *v = getenv("RECOMP_VIEWS_LOG");
        on = v && *v == '1';
    }
    return on;
}

/* The order of the rows: from the goalkeeper to the forwards (the lowest position bit), then from left
 * to right (the side bits 0x80 to 0x800), then the slot. */
static const Tactic *sort_tactic;
static int by_position(const void *a, const void *b)
{
    int x = *(const int *)a, y = *(const int *)b;
    uint16_t mx = sort_tactic->masks[x], my = sort_tactic->masks[y];
    int px = mx & 0x7f, py = my & 0x7f;
    px &= -px;
    py &= -py;
    if (px != py) return px - py;
    int sx = mx & 0xf80, sy = my & 0xf80;
    sx &= -sx;
    sy &= -sy;
    if (sx != sy) return sx - sy;
    return x - y;
}

static void log_row(uint32_t club, int chosen, int s, const Tactic *t, const Pick *p, const Depth *d)
{
    fprintf(stderr, "recomp: squad depth row club %#x tactic %d slot %d mask %x xi %d players", club, chosen - 1, s,
            t->masks[s], p->staff[s] ? (int32_t)rd32(g_mem, p->staff[s]) : -1);
    for (int j = 0; j < d->n[s]; j++)
        fprintf(stderr, " %d,%d,%d,%d,%d", (int32_t)rd32(g_mem, d->staff[s][j]), d->score[s][j], d->fit[s][j],
                d->out[s][j], views_shown_score(d->score[s][j]));
    fprintf(stderr, "\n");
}

void squad_depth_build(uint32_t club, uint32_t team, uint32_t buf, int x1, int y1, int x2, int y2, uint32_t bar)
{
    Tactic t;
    int chosen = views_formation(buf, bar, &t);
    if (!chosen) return;
    Pick p;
    static Depth d;
    picker_compute_view(club, team, 1, t.masks, views_human_manager(), &p);
    picker_depth(club, team, t.masks, views_human_manager(), &p, &d);

    int rows[SLOTS];
    for (int s = 0; s < SLOTS; s++) rows[s] = s;
    sort_tactic = &t;
    qsort(rows, SLOTS, sizeof *rows, by_position);

    /* A heading row and 11 rows: the position, then three cells of a name and a score. */
    int row = (y2 - y1) / (SLOTS + 1), text = (VIEWS_ROW_PX * 600 + (int)rt_screen_h - 1) / (int)rt_screen_h;
    if (text > row) text = row;
    /* The name and its score stay together: a name box of NAME_PX screen pixels at most, the score after it. */
    int pos_w = 60, cell = (x2 - x1 - pos_w) / DEPTH_PLAYERS, score_w = 30, name_w = cell - score_w - 8;
    int most = (NAME_PX * 800 + (int)rt_screen_w - 1) / (int)rt_screen_w;
    if (name_w > most) name_w = most;
    uint32_t head = rd16(g_mem, ITEM_HIGHLIGHT), plain = rd16(g_mem, LIST_TEXT), grey = rd16(g_mem, GREY);
    static const char *const titles[DEPTH_PLAYERS] = {"First choice", "Second choice", "Third choice"};
    int top = y1 + (row - text) / 2;
    views_text(buf, x1, top, x1 + pos_w - 4, top + text, head, 1, "Position");
    for (int j = 0; j < DEPTH_PLAYERS; j++)
        views_text(buf, x1 + pos_w + j * cell, top, x1 + pos_w + (j + 1) * cell - 8, top + text, head, 1, titles[j]);
    for (int r = 0; r < SLOTS; r++) {
        int s = rows[r], cy = y1 + (r + 1) * row + (row - text) / 2;
        char str[64];
        views_position_text(t.masks[s], str, sizeof str);
        views_text(buf, x1, cy, x1 + pos_w - 4, cy + text, head, 1, str);
        for (int j = 0; j < d.n[s]; j++) {
            int cx = x1 + pos_w + j * cell;
            views_short_name(d.staff[s][j], str, sizeof str);
            views_text(buf, cx, cy, cx + name_w, cy + text, d.out[s][j] ? grey : plain, 1, str);
            int shown = views_shown_score(d.score[s][j]);
            snprintf(str, sizeof str, "%d", shown);
            views_text(buf, cx + name_w, cy, cx + name_w + score_w, cy + text, columns_attribute_colour(shown), 0xc,
                       str);
        }
        if (log_on()) log_row(club, chosen, s, &t, &p, &d);
    }
    if (log_on()) fprintf(stderr, "recomp: squad depth drawn club %#x tactic %d rows %d\n", club, chosen - 1, SLOTS);
}
