/* The Squad depth tab of the Team views screen (spec 2026-10-15-features-0.4.0, slice 5; Q106, Q107, Q156).
 * views.c builds the frame and calls squad_depth_build for tab 2.
 *
 * One row for each position of the tactic chosen in the Formation drop-down (the same button, list and
 * node slot 2 as Best XI): the slots with the same position text share a row (Q161), so DL and DR are
 * apart and the two DC slots of a 4-4-2 are one row. A row of k slots holds the top k + 2 players from
 * picker_depth (picker.c): by the picker's value, best XI's players in those slots first of the available
 * players. So each name shows once in a row, and a player can appear in more than one row.
 * The players who cannot play the next match (injured, suspended, away with a national team, not
 * eligible) are in the rows too, with the name in grey. The number is the picker's score, 1 to 20, in the
 * attribute colour (Q156). A dark panel lies behind the table, so the background picture does not show
 * through (Q162).
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
#define PANEL 0x10a4u               /* RGB565 (16, 20, 32): the dark panel behind the table (Q162) */
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

static void log_row(uint32_t club, int chosen, const char *pos, int r, const int *row_of, const Pick *p,
                    const Depth *d)
{
    fprintf(stderr, "recomp: squad depth row club %#x tactic %d pos %s slots", club, chosen - 1, pos);
    for (int s = 0, k = 0; s < SLOTS; s++)
        if (row_of[s] == r) fprintf(stderr, "%s%d", k++ ? "," : " ", s);
    fprintf(stderr, " xi");
    for (int s = 0, k = 0; s < SLOTS; s++)
        if (row_of[s] == r)
            fprintf(stderr, "%s%d", k++ ? "," : " ", p->staff[s] ? (int32_t)rd32(g_mem, p->staff[s]) : -1);
    fprintf(stderr, " players");
    for (int j = 0; j < d->n[r]; j++)
        fprintf(stderr, " %d,%d,%d,%d,%d", (int32_t)rd32(g_mem, d->staff[r][j]), d->score[r][j], d->fit[r][j],
                d->out[r][j], views_shown_score(d->score[r][j]));
    fprintf(stderr, "\n");
}

void squad_depth_build(uint32_t club, uint32_t team, uint32_t buf, int x1, int y1, int x2, int y2, uint32_t bar)
{
    Tactic t;
    int chosen = views_formation(buf, bar, &t);
    if (!chosen) return;

    /* The rows: the slots in pitch order, a new row for each new position text. */
    int order[SLOTS], row_of[SLOTS], rows = 0, size[SLOTS] = {0};
    char pos[SLOTS][8];
    for (int s = 0; s < SLOTS; s++) order[s] = s;
    sort_tactic = &t;
    qsort(order, SLOTS, sizeof *order, by_position);
    for (int i = 0; i < SLOTS; i++) {
        int s = order[i], r = 0;
        char text[8];
        views_position_text(t.masks[s], text, sizeof text);
        while (r < rows && strcmp(pos[r], text)) r++;
        if (r == rows) memcpy(pos[rows++], text, sizeof text);
        row_of[s] = r;
        size[r]++;
    }
    int cells = 0;
    for (int r = 0; r < rows; r++)
        if (size[r] + 2 > cells) cells = size[r] + 2;

    Pick p;
    static Depth d;
    picker_compute_view(club, team, 1, t.masks, views_human_manager(), &p);
    picker_depth(club, team, t.masks, views_human_manager(), &p, row_of, rows, &d);

    /* The panel first, so the game draws the table over it. */
    views_box(buf, x1 - 6, y1 - 4, x2 + 6, y2 + 4, PANEL);
    if (log_on())
        fprintf(stderr, "recomp: squad depth panel %d %d %d %d colour %#x table %d %d %d %d\n", x1 - 6, y1 - 4, x2 + 6,
                y2 + 4, PANEL, x1, y1, x2, y2);

    /* A heading row and a row for each position: the position, then a cell of a name and a score for each
     * choice. Rows of at most 3 text heights, from the top. */
    int text = (VIEWS_ROW_PX * 600 + (int)rt_screen_h - 1) / (int)rt_screen_h, row = (y2 - y1) / (rows + 1);
    if (row > 3 * text) row = 3 * text;
    if (text > row) text = row;
    /* The name and its score stay together: a name box of NAME_PX screen pixels at most, the score after it. */
    int pos_w = 58, cell = (x2 - x1 - pos_w) / cells, score_w = 26, name_w = cell - score_w - 6;
    int most = (NAME_PX * 800 + (int)rt_screen_w - 1) / (int)rt_screen_w;
    if (name_w > most) name_w = most;
    uint32_t head = rd16(g_mem, ITEM_HIGHLIGHT), plain = rd16(g_mem, LIST_TEXT), grey = rd16(g_mem, GREY);
    static const char *const ordinal[DEPTH_PLAYERS] = {"1st", "2nd", "3rd", "4th", "5th", "6th", "7th", "8th", "9th",
                                                       "10th", "11th", "12th", "13th"};
    int top = y1 + (row - text) / 2;
    views_text(buf, x1, top, x1 + pos_w - 4, top + text, head, 1, "Position");
    for (int j = 0; j < cells; j++)
        views_text(buf, x1 + pos_w + j * cell, top, x1 + pos_w + j * cell + name_w, top + text, head, 1, ordinal[j]);
    for (int r = 0; r < rows; r++) {
        int cy = y1 + (r + 1) * row + (row - text) / 2;
        char str[64];
        views_text(buf, x1, cy, x1 + pos_w - 4, cy + text, head, 1, pos[r]);
        for (int j = 0; j < d.n[r]; j++) {
            int cx = x1 + pos_w + j * cell;
            views_short_name(d.staff[r][j], str, sizeof str);
            views_text(buf, cx, cy, cx + name_w, cy + text, d.out[r][j] ? grey : plain, 1, str);
            int shown = views_shown_score(d.score[r][j]);
            snprintf(str, sizeof str, "%d", shown);
            views_text(buf, cx + name_w, cy, cx + name_w + score_w, cy + text, columns_attribute_colour(shown), 0xc,
                       str);
        }
        if (log_on()) log_row(club, chosen, pos[r], r, row_of, &p, &d);
    }
    if (log_on()) fprintf(stderr, "recomp: squad depth drawn club %#x tactic %d rows %d\n", club, chosen - 1, rows);
}
