/* The Best XI tab of the Team views screen (spec 2026-10-15-features-0.4.0, slice 4; Q103 to Q106, Q128,
 * Q132). views.c builds the frame and calls best_xi_build for tab 1 and best_xi_input for each click.
 *
 * The eleven comes from the picker (picker_compute_view, picker.c), the same function that "Pick best
 * team" uses, but it is never stored: the tab only reads the game (Q104). The picker leaves out the
 * players who cannot play the next match (injured, suspended, away with a national team), as the squad
 * screen does. For another club (Q108) the picker counts only the attributes that the player's screens
 * show (0x543280), so no hidden value feeds the pick or a number on the screen (Q111).
 *
 * Each player is a box with his name in the club's colours, on the pitch at the place of his slot's
 * position mask, and his score for the slot under it in the attribute colour: the picker's score, the
 * average of the key shown attributes of the position times fit / 20, rounded, 1 to 20 (Q156).
 *
 * The Formation drop-down in the heading lists the tactics of the game's own Tactics drop-down (tactics.c).
 * Item 0 is the player's current tactic, where the tab opens. A choice sets node slot 2 and builds the
 * screen again; the eleven is then picked for the chosen tactic's slots. The team does not change. */
#include "win.h"
#include "views.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define SCREENS 0xb74450u           /* the screen stack S */
#define NODE_SLOT 0x7ff010u         /* thiscall on S (slot) */
#define SET_SLOT 0x7ff270u          /* thiscall on S (slot, value, copy) */
#define AREA_CREATE 0x55fba0u       /* thiscall on a Display, 11 args */
#define WIDGET_CREATE 0x55f990u     /* thiscall on a Display, 18 args */
#define BUTTON 0x804030u            /* thiscall on S, 12 args: a button with its own rectangle (0x456464) */
#define DISPLAYS 0xb74c4cu          /* the Display of a player slot, 0x300 bytes apart */
#define PLAYER_SLOT 0xb77c7au       /* s16: the player slot that has the turn */
#define PLAYER_MANAGER 0xb74c26u    /* per player slot, 0x300 bytes apart: the manager */
#define STORE 0xde1d08u             /* the stored selections (picker.c) */
#define POPUP_COLOUR 0xae31aau      /* u16 colours of the Print items (0x4564ed) */
#define ITEM_HIGHLIGHT 0xaebdecu
#define ITEM_TEXT 0xaebdeau
#define BUTTON_TEXT 0xae3184u
#define LIST_TEXT 0xaebde8u         /* the text colour of the squad list's cells (0x45c781) */       /* the Print button's text colour when the bar colour is ITEM_TEXT */

#define FORMATION_ID 0x62u          /* the Formation items; data 1 + the index in the list */
#define CHOSEN 2                    /* node slot: the index of the chosen tactic in the list */
#define FORM_PAGE 3                 /* node slot: the page of the Formation list */
#define PAGE_ITEM 100u              /* data of a page item: PAGE_ITEM + the page */
#define ITEM_PX 21                  /* a popup item, screen pixels: the items do not scale */
#define BUTTON_BOTTOM 35            /* the Formation button's lower edge, classic numbers */
#define SLOTS 11

static uint32_t call_this(uint32_t fn, uint32_t self, int n, const uint32_t *args)
{
    Cpu saved = cpu;
    cpu.ecx = self;
    uint32_t r = rt_callback(fn, n, args);
    cpu = saved;
    return r;
}

static uint32_t node_slot(int n)
{
    uint32_t a[1] = {(uint32_t)n};
    return call_this(NODE_SLOT, SCREENS, 1, a);
}

static void set_slot(int n, uint32_t value)
{
    uint32_t a[3] = {(uint32_t)n, value, 0};
    call_this(SET_SLOT, SCREENS, 3, a);
}

static uint32_t display(void)
{
    return rd32(g_mem, DISPLAYS + (uint32_t)(int32_t)(int16_t)rd16(g_mem, PLAYER_SLOT) * 0x300u);
}

static uint32_t human_manager(void)
{
    return rd32(g_mem, PLAYER_MANAGER + (uint32_t)(int32_t)(int16_t)rd16(g_mem, PLAYER_SLOT) * 0x300u);
}

/* A widget at a rectangle of the 800x600 screen with a text; the widget create copies the text. flags:
 * the text flags, 0xc centred (the tabs), 1 left (the popup items). */
static void text_widget(uint32_t buf, int x1, int y1, int x2, int y2, uint32_t style, uint32_t colour,
                        uint32_t text_colour, uint32_t flags, const char *text)
{
    win_put_str(buf, text, VIEWS_TEXT_SIZE);
    uint32_t w[18] = {2, (uint32_t)x1, (uint32_t)y1, (uint32_t)x2, (uint32_t)y2, 0, 0, style, colour, colour, flags, 1,
                      text_colour, buf, 0, 0, 0, 0xffffffffu};
    call_this(WIDGET_CREATE, display(), 18, w);
}

/* The short name of a player: the common name, else the second name (staff +0xc, +0x8). */
static void short_name(uint32_t staff, char *out, size_t size)
{
    uint32_t common = rd32(g_mem, staff + 0xc), second = rd32(g_mem, staff + 0x8);
    uint32_t at = common && g_mem[common] ? common : second;
    snprintf(out, size, "%s", at ? (const char *)(g_mem + at) : "");
}

/* The place of a slot on the pitch, from its position mask: the row from the position bits (GK at the
 * bottom, F at the top), the column from the side bits (0x80 left to 0x800 right). With more than one bit,
 * the average. Rows and columns in 1/1000 of the pitch. */
static void place(uint16_t mask, int *across, int *down)
{
    static const struct { uint16_t bit; int down; } rows[] = {
        {0x1, 925}, {0x2, 790}, {0x4, 700}, {0x8, 570}, {0x10, 445}, {0x20, 310}, {0x40, 160}};
    static const struct { uint16_t bit; int across; } sides[] = {
        {0x80, 100}, {0x100, 300}, {0x200, 500}, {0x400, 700}, {0x800, 900}};
    int n = 0, sum = 0;
    for (unsigned i = 0; i < sizeof rows / sizeof *rows; i++)
        if (mask & rows[i].bit) { sum += rows[i].down; n++; }
    *down = n ? sum / n : 500;
    n = sum = 0;
    for (unsigned i = 0; i < sizeof sides / sizeof *sides; i++)
        if (mask & sides[i].bit) { sum += sides[i].across; n++; }
    *across = n ? sum / n : 500;
}

/* By row, then across: the order in which the slots of one row spread over the pitch. */
typedef struct { int slot, across, down; } Spot;
static int by_place(const void *a, const void *b)
{
    const Spot *x = a, *y = b;
    if (x->down != y->down) return x->down - y->down;
    if (x->across != y->across) return x->across - y->across;
    return x->slot - y->slot;
}

/* The selection that the game stores for the club's first team, as staff indices (-1 for none): the
 * record [STORE + 0x601] + club index * 0x91, slots at +2 (0x8968f0, 0x896a90). For the log only. */
static void stored(uint32_t club, int *out)
{
    uint32_t record = rd32(g_mem, rd32(g_mem, STORE) + 0x601) + rd32(g_mem, club) * 0x91u;
    for (int s = 0; s < SLOTS; s++) out[s] = (int32_t)rd32(g_mem, record + 2 + 4 * (uint32_t)s);
}

static int log_on(void)
{
    static int on = -1;
    if (on < 0) {
        const char *v = getenv("RECOMP_VIEWS_LOG");
        on = v && *v == '1';
    }
    return on;
}

/* The number on the screen for a picker value (Pick.score): the score value / 1200, rounded, 1 to 20.
 * 0 for a player with no score (every key attribute hidden): the screen shows "-". */
static int shown_score(int value) { return pick_shown(value); }

/* The text and colour of a score: "-" in the list's text colour when there is none. */
static void score_text(int shown, char *out, size_t size, uint32_t *colour)
{
    if (shown) snprintf(out, size, "%d", shown);
    else snprintf(out, size, "-");
    *colour = shown ? columns_attribute_colour(shown) : rd16(g_mem, LIST_TEXT);
}

static void log_draw(uint32_t club, int chosen, const Tactic *t, const Pick *p)
{
    if (!log_on()) return;
    int sel[SLOTS];
    stored(club, sel);
    fprintf(stderr, "recomp: best xi club %#x tactic %d \"%s\" masks", club, chosen, t->name);
    for (int s = 0; s < SLOTS; s++) fprintf(stderr, " %x", t->masks[s]);
    fprintf(stderr, " staff");
    for (int s = 0; s < SLOTS; s++) fprintf(stderr, " %d", p->staff[s] ? (int32_t)rd32(g_mem, p->staff[s]) : -1);
    fprintf(stderr, " fit");
    for (int s = 0; s < SLOTS; s++) fprintf(stderr, " %d", p->fit[s]);
    fprintf(stderr, " selection");
    for (int s = 0; s < SLOTS; s++) fprintf(stderr, " %d", sel[s]);
    fprintf(stderr, " score");
    for (int s = 0; s < SLOTS; s++) fprintf(stderr, " %d", p->score[s]);
    fprintf(stderr, " shown");
    for (int s = 0; s < SLOTS; s++) fprintf(stderr, " %d", shown_score(p->score[s]));
    fprintf(stderr, "\n");
}

/* The tactics that one page of the Formation popup holds. A popup holds at most 30 widgets (0x4033fa) and
 * opens under its button when it fits above the screen's bottom (0x4039b5), so the list goes in pages
 * of 28 at most, with an item for the page before and the page after, as the squad drop-down of the
 * Compare tab does. RECOMP_VIEWS_FORMATION_PAGE sets the size for a test. */
static int formation_page_size(void)
{
    const char *v = getenv("RECOMP_VIEWS_FORMATION_PAGE");
    if (v && atoi(v) > 0) return atoi(v) > 28 ? 28 : atoi(v);
    int bottom = (BUTTON_BOTTOM * (int)rt_screen_h + 599) / 600;
    int n = ((int)rt_screen_h - bottom - 12) / ITEM_PX - 2;
    return n > 28 ? 28 : n < 4 ? 4 : n;
}

static void formation_item(uint32_t buf, uint32_t popup, const char *text, uint32_t data)
{
    win_put_str(buf, text, VIEWS_TEXT_SIZE);
    uint32_t w[18] = {2, 0, 0, 0, 0, 0, 0, 0x10, rd16(g_mem, POPUP_COLOUR), rd16(g_mem, ITEM_HIGHLIGHT), 1, 1,
                      rd16(g_mem, ITEM_TEXT), buf, 0, FORMATION_ID, data, popup};
    call_this(WIDGET_CREATE, display(), 18, w);
}

/* The Formation button in the heading, as the Print button of the club screen (0x45641b..0x4566a6), with
 * the chosen tactic's name, and its drop-down with the tactics of the list, a page at a time when the
 * list does not fit. */
static void formation(uint32_t buf, const Tactic *list, int n, int chosen, uint32_t bar)
{
    uint32_t fg = bar == rd16(g_mem, ITEM_TEXT) ? rd16(g_mem, BUTTON_TEXT) : rd16(g_mem, ITEM_TEXT);
    win_put_str(buf, list[chosen].name, VIEWS_TEXT_SIZE);
    uint32_t b[12] = {VIEWS_FORMATION_X1, 15, 785, BUTTON_BOTTOM, 0x30, bar, 0xc, 1, fg, buf, 0, 0};
    uint32_t button = (uint32_t)(int32_t)(int16_t)call_this(BUTTON, SCREENS, 12, b);
    uint32_t a[11] = {0, 0, 0, 0, 0, 0, 0, 0, 0x130, rd16(g_mem, POPUP_COLOUR), button};
    uint32_t popup = (uint32_t)(int32_t)(int16_t)call_this(AREA_CREATE, display(), 11, a);
    int size = formation_page_size(), paged = n > size, page = paged ? (int)node_slot(FORM_PAGE) : 0;
    if (!paged) size = n;
    else if (page < 0 || page * size >= n) page = 0;
    if (paged && page > 0) formation_item(buf, popup, "Earlier formations", PAGE_ITEM + (uint32_t)page - 1);
    for (int i = page * size; i < n && i < (page + 1) * size; i++)
        formation_item(buf, popup, list[i].name, (uint32_t)i + 1);
    if (paged && (page + 1) * size < n) formation_item(buf, popup, "More formations", PAGE_ITEM + (uint32_t)page + 1);
}

/* A length of screen pixels in classic numbers, across and down (the widget create scales them back). */
static int across_px(int px) { return (px * 800 + (int)rt_screen_w - 1) / (int)rt_screen_w; }
static int down_px(int px) { return (px * 600 + (int)rt_screen_h - 1) / (int)rt_screen_h; }

/* The players: a name box and the score under it for each slot. x1..x2, y1..y2: the pitch as drawn, in
 * classic numbers (views_pitch_rect). Every box stays inside the pitch (Q154). */
static void players(uint32_t buf, const Tactic *t, const Pick *p, int x1, int y1, int x2, int y2, uint32_t bar,
                    uint32_t fg)
{
    Spot spots[SLOTS];
    for (int s = 0; s < SLOTS; s++) {
        spots[s].slot = s;
        place(t->masks[s], &spots[s].across, &spots[s].down);
    }
    qsort(spots, SLOTS, sizeof *spots, by_place);
    int w = x2 - x1, h = y2 - y1, box = across_px(VIEWS_BOX_PX), half = box / 2, gap = across_px(2);
    int lo = x1 + gap + half, hi = x2 - gap - (box - half);
    if (log_on()) fprintf(stderr, "recomp: best xi pitch %d %d %d %d boxes", x1, y1, x2, y2);
    for (int i = 0; i < SLOTS;) {
        int j = i;
        while (j < SLOTS && spots[j].down == spots[i].down) j++;
        /* A row of k players: each at the place of its side when no two boxes overlap there, else the
         * row spreads evenly over the pitch in the same order. */
        int k = j - i, cxs[SLOTS], spread = 0;
        for (int m = i; m < j; m++) {
            int cx = x1 + w * spots[m].across / 1000;
            cxs[m - i] = cx < lo ? lo : cx > hi ? hi : cx;
            if (m > i && cxs[m - i] - cxs[m - i - 1] < box + gap) spread = 1;
        }
        for (int m = i; m < j; m++) {
            int s = spots[m].slot, cx = cxs[m - i];
            if (spread) cx = k > 1 ? lo + (hi - lo) * (m - i) / (k - 1) : (lo + hi) / 2;
            int cy = y1 + h * spots[m].down / 1000;
            char name[64];
            if (p->staff[s]) short_name(p->staff[s], name, sizeof name);
            else snprintf(name, sizeof name, "-");
            text_widget(buf, cx - half, cy - 14, cx - half + box, cy + 1, 0x30, bar, fg, 0xc, name);
            if (log_on()) fprintf(stderr, " %d %d", cx - half, cx - half + box);
            if (p->staff[s]) {
                char r[8];
                uint32_t colour;
                score_text(shown_score(p->score[s]), r, sizeof r, &colour);
                text_widget(buf, cx - half, cy + 2, cx - half + box, cy + 15, 0, 0, colour, 0xc, r);
            }
        }
        i = j;
    }
    if (log_on()) fprintf(stderr, "\n");
}

/* The position text of a position bit with its side bits: GK, SW, then D, DM, M, AM or F with L, C or R. */
static void position_text(uint16_t mask, char *out, size_t size)
{
    static const struct { uint16_t bit; const char *name; } names[] = {
        {0x1, "GK"}, {0x2, "SW"}, {0x4, "D"}, {0x8, "DM"}, {0x10, "M"}, {0x20, "AM"}, {0x40, "F"}};
    const char *name = "-", *side = "";
    for (unsigned i = 0; i < sizeof names / sizeof *names; i++)
        if (mask & names[i].bit) { name = names[i].name; break; }
    if (mask & 0x3) side = "";
    else if (mask & 0x80) side = "L";
    else if (mask & 0x800) side = "R";
    else if (mask & 0x700) side = "C";
    snprintf(out, size, "%s%s", name, side);
}

/* The list beside the pitch (Q155): the subs of the pick, then the other players who can play, each with
 * the position and the score of his best slot of the chosen formation. Two columns, rows of VIEWS_ROW_PX
 * screen pixels, in the panel x1..y2. */
static void bench_list(uint32_t buf, const Pick *p, int x1, int y1, int x2, int y2)
{
    int row = down_px(VIEWS_ROW_PX), rows = (y2 - y1) / row, col_w = (x2 - x1) / 2;
    int pos_w = across_px(34), fit_w = across_px(24);
    uint32_t text = rd16(g_mem, LIST_TEXT), head = rd16(g_mem, ITEM_HIGHLIGHT);
    int subs = 0;
    for (int s = SLOTS; s < p->slots; s++) subs += p->staff[s] != 0;
    int r = 0, shown = 0;
    for (int i = 0; i < p->bench && r < 2 * rows; i++) {
        if (i == 0 || i == subs) {             /* a heading before each part, never last in a column */
            if (r % rows == rows - 1) r++;
            int cx = x1 + (r / rows) * col_w, cy = y1 + (r % rows) * row;
            text_widget(buf, cx, cy, cx + col_w - 4, cy + row - 1, 0, 0, head, 1,
                        i < subs ? "Substitutes" : "Other players");
            if (++r >= 2 * rows) break;
        }
        int cx = x1 + (r / rows) * col_w, cy = y1 + (r % rows) * row;
        char t[64];
        position_text(p->bench_mask[i], t, sizeof t);
        text_widget(buf, cx, cy, cx + pos_w, cy + row - 1, 0, 0, head, 1, t);
        short_name(p->bench_staff[i], t, sizeof t);
        text_widget(buf, cx + pos_w, cy, cx + col_w - fit_w - 4, cy + row - 1, 0, 0, text, 1, t);
        uint32_t colour;
        score_text(shown_score(p->bench_score[i]), t, sizeof t, &colour);
        text_widget(buf, cx + col_w - fit_w - 4, cy, cx + col_w - 4, cy + row - 1, 0, 0, colour, 0xc, t);
        r++;
        shown++;
    }
    if (log_on()) {
        fprintf(stderr, "recomp: best xi bench subs");
        for (int s = SLOTS; s < p->slots; s++)
            if (p->staff[s]) fprintf(stderr, " %d", (int32_t)rd32(g_mem, p->staff[s]));
        fprintf(stderr, " list");
        for (int i = 0; i < p->bench; i++) fprintf(stderr, " %d", (int32_t)rd32(g_mem, p->bench_staff[i]));
        fprintf(stderr, " fit");
        for (int i = 0; i < p->bench; i++) fprintf(stderr, " %d", p->bench_fit[i]);
        fprintf(stderr, " score");
        for (int i = 0; i < p->bench; i++) fprintf(stderr, " %d", p->bench_score[i]);
        fprintf(stderr, " numbers");
        for (int i = 0; i < p->bench; i++) fprintf(stderr, " %d", shown_score(p->bench_score[i]));
        fprintf(stderr, " shown %d\n", shown);
    }
}

int best_xi_build(uint32_t club, uint32_t team, uint32_t buf, int x1, int y1, int x2, int y2, int lx1, int ly1,
                  int lx2, int ly2, uint32_t bar, uint32_t fg)
{
    static Tactic list[VIEWS_TACTICS];
    int n = tactics_list(human_manager(), list, VIEWS_TACTICS);
    if (n <= 0) return 0;
    int chosen = (int)node_slot(CHOSEN);
    if (chosen < 0 || chosen >= n) chosen = 0;
    formation(buf, list, n, chosen, bar);
    static int listed;
    if (!listed++ && getenv("RECOMP_VIEWS_LOG")) fprintf(stderr, "recomp: best xi tactics %d current 0\n", n);
    Pick p;
    picker_compute_view(club, team, 1, list[chosen].masks, human_manager(), &p);
    log_draw(club, chosen, &list[chosen], &p);
    players(buf, &list[chosen], &p, x1, y1, x2, y2, bar, fg);
    bench_list(buf, &p, lx1, ly1, lx2, ly2);
    return n;
}

int best_xi_input(uint32_t id, uint32_t data)
{
    if (id != FORMATION_ID || data < 1) return 0;
    if (data >= PAGE_ITEM) set_slot(FORM_PAGE, data - PAGE_ITEM);    /* a page item: only the page changes */
    else set_slot(CHOSEN, data - 1);
    return 1;
}

/* For the Squad depth tab (squad_depth.c): the same Formation button and list, the same texts. */
int views_formation(uint32_t buf, uint32_t bar, Tactic *chosen_out)
{
    static Tactic list[VIEWS_TACTICS];
    int n = tactics_list(human_manager(), list, VIEWS_TACTICS);
    if (n <= 0) return 0;
    int chosen = (int)node_slot(CHOSEN);
    if (chosen < 0 || chosen >= n) chosen = 0;
    formation(buf, list, n, chosen, bar);
    *chosen_out = list[chosen];
    return chosen + 1;
}

void views_text(uint32_t buf, int x1, int y1, int x2, int y2, uint32_t text_colour, uint32_t flags, const char *text)
{
    text_widget(buf, x1, y1, x2, y2, 0, 0, text_colour, flags, text);
}

void views_box(uint32_t buf, int x1, int y1, int x2, int y2, uint32_t colour)
{
    text_widget(buf, x1, y1, x2, y2, 0x30, colour, colour, 0xc, "");
}

void views_short_name(uint32_t staff, char *out, size_t size) { short_name(staff, out, size); }
int views_shown_score(int value) { return shown_score(value); }
void views_score_text(int shown, char *out, size_t size, uint32_t *colour) { score_text(shown, out, size, colour); }
void views_position_text(uint16_t mask, char *out, size_t size) { position_text(mask, out, size); }
uint32_t views_human_manager(void) { return human_manager(); }
