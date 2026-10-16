/* The comparison of two players with the polygon (#22, spec 2026-10-15-features-0.4.0, slice 6; Q110,
 * Q111, Q114 to Q117). Layout variant B of Max's prototype: player 1's attributes on the left, the
 * polygon in the middle, player 2's attributes on the right.
 *
 * Two screens build it: the original's "Compare two chosen players" screen (compare_screen.c) and the
 * Compare players tab of the Team views screen (views.c), where the name box of each side is a drop-down
 * with the squad.
 *
 * - Each side lists the profile's attribute groups (compare_layout.c, as the profile with
 *   attribute_groups on), the numbers in the attribute colours. A white frame marks the higher number of
 *   each pair. When the rows do not fit in one column (classic mode), the groups go in two columns.
 * - The polygon has eight axes, each the average of its shown attributes; two goalkeepers get the
 *   goalkeeper axes. Player 1 is blue, player 2 orange; the shapes overlap with alpha. Under the polygon a
 *   table gives the value of each axis and is the legend.
 * - Only shown values: the profile's attribute getters (1 to 20). An attribute that the game masks for the
 *   viewer shows "-" and does not count (Q111).
 *
 * The polygon, its grid and the frames go on the game's draw surface after the game draws their widgets
 * (compare_widget_drawn, from views_widget_drawn), so the screen dumps hold them too. The code only reads
 * the game's data. */
#include "win.h"
#include "draw.h"
#include "views.h"
#include "compare_layout.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define SCREENS 0xb74450u           /* the screen stack S */
#define TOP_NODE (SCREENS + 0x7d4u + 0x0eu)
#define AREA_CREATE 0x55fba0u       /* thiscall on a Display, 11 args */
#define WIDGET_CREATE 0x55f990u     /* thiscall on a Display, 18 args */
#define BUTTON 0x804030u            /* thiscall on S, 12 args: a button with its own rectangle */
#define DISPLAYS 0xb74c4cu          /* the Display of a player slot, 0x300 bytes apart */
#define PLAYER_SLOT 0xb77c7au       /* s16: the player slot that has the turn */
#define PLAYER_MANAGER 0xb74c26u    /* per player slot, 0x300 bytes apart: the manager */
#define WIDGETS 0xba95eu            /* Display + this: widget 0; 0x18c bytes each */
#define WIDGET_SIZE 0x18cu
#define NO_DRAW 0xaebe0cu           /* not 0: the draw functions of the original draw nothing */
#define DRAW_SURFACE 0xaebde4u      /* the surface that the draw functions of the original draw on */
#define MASKING 0x9cc270u           /* byte: not 0 when the game masks attributes of unknown players */
#define SHOWN_MASKED 0x543440u      /* cdecl (viewer manager, staff, attribute): the shown value, < 0 masked */
#define SHOWN 0x543540u             /* cdecl (staff, player data, attribute): the shown value */
#define ATTRIBUTE_NAME 0x5428a0u    /* cdecl (attribute 1..31, text, size, 0): the attribute's name */
#define POSITIONS 0x53ee50u         /* cdecl (player data, text, size, 1): the positions text, "GK" first */
#define CLUB_NAME 0x542100u         /* cdecl (club, team, buffer, size >= 0x47, mode, flag); mode 1: short */
#define TEXT_WIDTH 0x5e5ad0u        /* cdecl (font, text): the width in screen pixels */
#define POPUP_COLOUR 0xae31aau      /* u16 colours of the popup items (best_xi.c) */
#define ITEM_HIGHLIGHT 0xaebdecu    /* yellow: headings */
#define ITEM_TEXT 0xaebdeau
#define LIST_TEXT 0xaebde8u         /* the text colour of the squad list's cells */

#define SQUAD 0xd7u                 /* club: 50 staff pointers */
#define PLAYER_DATA 0x61u           /* staff: the player data, 0 for no player */
#define CLUB 0x39u                  /* staff: the club */
#define AGE 0x18u                   /* staff: s16 age */

#define CHOOSE1_ID 0x63u            /* the drop-down items of the Compare tab; data: the squad index + 1 */
#define CHOOSE2_ID 0x64u
#define PROFILE_ID 0x7d7u           /* a link to a staff profile, data the staff record (0x764b2a) */

#define BLUE 0x4cdfu                /* player 1: (74, 155, 255) */
#define ORANGE 0xfcc5u              /* player 2: (255, 154, 46) */
#define GRID 0x2951u                /* the rings and spokes: (42, 42, 138) */
#define WHITE 0xffffu
#define DARK 0x0841u                /* the text in the name boxes */
#define FILL_ALPHA 77               /* 0.3 of 256 */
#define BACKDROP 0x0867u            /* the middle panel's fill: (10, 15, 60), the screen colour of the prototype */
#define BACKDROP_ALPHA 224

#define FONT 1
#define ROW_PX VIEWS_ROW_PX         /* a row of a list, screen pixels */
#define GAP_PX 8
#define MARKS 64
#define SMALL_PX 80                /* a smaller polygon radius gets the short axis names */

typedef struct {
    uint32_t staff, data;
    int keeper;
    uint8_t attr[32];               /* shown values, 0 = hidden */
    char name[64], line[96];
} Player;

/* What the hook needs of the comparison built last. */
static struct {
    uint32_t build;                 /* the build function of the screen that made it, 0 for none */
    int polygon;                    /* the polygon's widget */
    int radius;                     /* screen pixels */
    int value[2][COMPARE_AXES];     /* tenths */
    int marks, mark[MARKS];         /* the widgets that get a white frame */
    int backdrop;                   /* the widget over the middle panel that gets the dark fill (Q158) */
} shown;

static uint32_t buf;                /* text buffer in the game's memory, VIEWS_TEXT_SIZE bytes */

static uint32_t call_this(uint32_t fn, uint32_t self, int n, const uint32_t *args)
{
    Cpu saved = cpu;
    cpu.ecx = self;
    uint32_t r = rt_callback(fn, n, args);
    cpu = saved;
    return r;
}

static uint32_t call(uint32_t fn, int n, const uint32_t *args)
{
    Cpu saved = cpu;
    uint32_t r = rt_callback(fn, n, args);
    cpu = saved;
    return r;
}

static uint32_t display(void)
{
    return rd32(g_mem, DISPLAYS + (uint32_t)(int32_t)(int16_t)rd16(g_mem, PLAYER_SLOT) * 0x300u);
}

static uint32_t human_manager(void)
{
    return rd32(g_mem, PLAYER_MANAGER + (uint32_t)(int32_t)(int16_t)rd16(g_mem, PLAYER_SLOT) * 0x300u);
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

/* Screen pixels to 800x600 numbers and back, as the widget create scales (display.c). */
static int sx(int x) { return x * (int)rt_screen_w / 800; }
static int sy(int y) { return y * (int)rt_screen_h / 600; }
static int cx_(int px) { return (px * 800 + (int)rt_screen_w - 1) / (int)rt_screen_w; }
static int cy_(int px) { return (px * 600 + (int)rt_screen_h - 1) / (int)rt_screen_h; }

/* A widget at a rectangle in screen pixels with a text. flags: 1 left, 0x40 right, 0xc centred. style
 * 0x30 fills the box with colour. Returns the widget's index. */
static int text_at(int x1, int y1, int x2, int y2, uint32_t style, uint32_t colour, uint32_t text_colour,
                   uint32_t flags, const char *text, uint32_t id, uint32_t data)
{
    win_put_str(buf, text, VIEWS_TEXT_SIZE);
    uint32_t w[18] = {2, (uint32_t)cx_(x1), (uint32_t)cy_(y1), (uint32_t)cx_(x2), (uint32_t)cy_(y2), 0, 0, style,
                      colour, colour, flags, FONT, text_colour, buf, 0, id, data, 0xffffffffu};
    return (int16_t)call_this(WIDGET_CREATE, display(), 18, w);
}

static void panel_at(int x1, int y1, int x2, int y2)
{
    uint32_t a[11] = {(uint32_t)cx_(x1), (uint32_t)cy_(y1), (uint32_t)cx_(x2), (uint32_t)cy_(y2), 1, 0, 1, 0, 2, 0,
                      0xffffffffu};
    call_this(AREA_CREATE, display(), 11, a);
}

static int width_of(const char *text)
{
    win_put_str(buf, text, VIEWS_TEXT_SIZE);
    uint32_t a[2] = {FONT, buf};
    return (int32_t)call(TEXT_WIDTH, 2, a);
}

static void attribute_name(int k, char *out, size_t size)
{
    wr8(g_mem, buf, 0);
    uint32_t a[4] = {(uint32_t)k, buf, VIEWS_TEXT_SIZE, 0};
    call(ATTRIBUTE_NAME, 4, a);
    snprintf(out, size, "%s", win_str(buf));
}

static void positions(uint32_t data, char *out, size_t size)
{
    wr8(g_mem, buf, 0);
    uint32_t a[4] = {data, buf, VIEWS_TEXT_SIZE, 1};
    call(POSITIONS, 4, a);
    snprintf(out, size, "%s", win_str(buf));
}

/* The name as the squad list shows it: the common name, else the first and second names. */
static void full_name(uint32_t staff, char *out, size_t size)
{
    uint32_t common = rd32(g_mem, staff + 0xc), first = rd32(g_mem, staff + 0x4), second = rd32(g_mem, staff + 0x8);
    if (common && g_mem[common]) snprintf(out, size, "%s", win_str(common));
    else snprintf(out, size, "%s%s%s", first ? win_str(first) : "", first && second ? " " : "",
                  second ? win_str(second) : "");
}

/* The common name, else the second name, as best XI shows it. */
static void short_name(uint32_t staff, char *out, size_t size)
{
    if (!staff) {                   /* a squad of one player */
        snprintf(out, size, "-");
        return;
    }
    uint32_t common = rd32(g_mem, staff + 0xc), second = rd32(g_mem, staff + 0x8);
    uint32_t at = common && g_mem[common] ? common : second;
    snprintf(out, size, "%s", at ? win_str(at) : "");
}

static void load(Player *p, uint32_t staff)
{
    memset(p, 0, sizeof *p);
    p->staff = staff;
    p->data = staff ? rd32(g_mem, staff + PLAYER_DATA) : 0;
    if (!p->data) return;
    char pos[64];
    positions(p->data, pos, sizeof pos);
    p->keeper = !strncmp(pos, "GK", 2);
    /* The getters of the profile and the original comparison (0x87743f..0x8774b6). */
    uint32_t viewer = human_manager();
    int masking = rd8(g_mem, MASKING) != 0;
    for (uint32_t k = 1; k <= 31; k++) {
        uint32_t a[3] = {viewer, staff, k}, b[3] = {staff, p->data, k};
        int v = (int8_t)(masking ? call(SHOWN_MASKED, 3, a) : call(SHOWN, 3, b));
        p->attr[k] = (uint8_t)(v < 0 ? 0 : v > 20 ? 20 : v);
    }
    full_name(staff, p->name, sizeof p->name);
    char club[0x80] = "";
    uint32_t c = rd32(g_mem, staff + CLUB);
    if (c) {
        wr8(g_mem, buf, 0);
        uint32_t a[6] = {c, 0, buf, VIEWS_TEXT_SIZE, 1, 1};
        call(CLUB_NAME, 6, a);
        snprintf(club, sizeof club, "%s", win_str(buf));
    }
    snprintf(p->line, sizeof p->line, "%s%s%s, age %d", club, *club ? ", " : "", pos,
             (int16_t)rd16(g_mem, staff + AGE));
}

/* The squad drop-down of the Compare tab on the name box x1..y2 (screen pixels): a button with the
 * player's name and a popup with the club's players. A popup holds at most 30 widgets (0x4033fa). It opens
 * under its button when it fits above the screen's bottom (0x4039b5), else over the button, where it must
 * not pass the screen's top (0x403b73). Its items do not scale (21 pixels each). So the squad shows in
 * pages that fit under the name box, with an item for the page before and the page after. */
#define ITEM_PX 21
#define PAGE_ITEM 100u              /* data of a page item: PAGE_ITEM + the page */
static int page_size(int bottom)
{
    int n = ((int)rt_screen_h - bottom - 12) / ITEM_PX - 2;
    return n > 28 ? 28 : n < 4 ? 4 : n;
}

static void item(uint32_t popup, const char *text, uint32_t id, uint32_t data)
{
    win_put_str(buf, text, VIEWS_TEXT_SIZE);
    uint32_t w[18] = {2, 0, 0, 0, 0, 0, 0, 0x10, rd16(g_mem, POPUP_COLOUR), rd16(g_mem, ITEM_HIGHLIGHT), 1, FONT,
                      rd16(g_mem, ITEM_TEXT), buf, 0, id, data, popup};
    call_this(WIDGET_CREATE, display(), 18, w);
}

static void chooser(int x1, int y1, int x2, int y2, uint32_t colour, const char *name, uint32_t club, uint32_t id,
                    int page)
{
    win_put_str(buf, name, VIEWS_TEXT_SIZE);
    uint32_t b[12] = {(uint32_t)cx_(x1), (uint32_t)cy_(y1), (uint32_t)cx_(x2), (uint32_t)cy_(y2), 0x30, colour, 0xc,
                      FONT, DARK, buf, 0, 0};
    uint32_t button = (uint32_t)(int32_t)(int16_t)call_this(BUTTON, SCREENS, 12, b);
    uint32_t a[11] = {0, 0, 0, 0, 0, 0, 0, 0, 0x130, rd16(g_mem, POPUP_COLOUR), button};
    uint32_t popup = (uint32_t)(int32_t)(int16_t)call_this(AREA_CREATE, display(), 11, a);
    uint32_t list[50];
    int n = 0;
    for (uint32_t i = 0; i < 50; i++) {
        uint32_t staff = rd32(g_mem, club + SQUAD + 4 * i);
        if (staff && rd32(g_mem, staff + PLAYER_DATA)) list[n++] = i;
    }
    int size = page_size(y2);
    if (page < 0 || page * size >= n) page = 0;
    if (page > 0) item(popup, "Earlier players", id, PAGE_ITEM + (uint32_t)page - 1);
    for (int i = page * size; i < n && i < (page + 1) * size; i++) {
        char t[64];
        full_name(rd32(g_mem, club + SQUAD + 4 * list[i]), t, sizeof t);
        item(popup, t, id, list[i] + 1);
    }
    if ((page + 1) * size < n) item(popup, "More players", id, PAGE_ITEM + (uint32_t)page + 1);
}

/* The groups of the rows: start and count of each group (a heading and its attributes). */
typedef struct { int start, count; } Group;

static int groups_of(const int *rows, int n, Group *g)
{
    int k = 0;
    for (int i = 0; i < n; i++) {
        if (groups_heading(rows[i]) || !k) g[k++] = (Group){i, 0};
        g[k - 1].count++;
    }
    return k;
}

/* Shares the groups over two columns with the lowest tallest column, the groups of each column in their
 * order. Returns the bit mask of the groups in column 2. */
static unsigned two_columns(const Group *g, int k)
{
    unsigned best = 0;
    int best_h = 1 << 30;
    for (unsigned m = 0; m < (1u << k); m++) {
        int h[2] = {0, 0};
        for (int i = 0; i < k; i++) h[(m >> i) & 1] += g[i].count;
        int tall = h[0] > h[1] ? h[0] : h[1];
        if (tall < best_h || (tall == best_h && m < best)) { best_h = tall; best = m; }
    }
    return best;
}

typedef struct {
    int keepers, n, rows[COMPARE_ROWS], k, columns;
    Group g[8];
    unsigned second;                /* the groups in column 2 */
    int name_w, value_w;            /* screen pixels */
    char names[COMPARE_ROWS][48];
} Sides;

static int side_width(const Sides *s) { return s->columns * (s->name_w + s->value_w) + (s->columns - 1) * GAP_PX + 8; }

/* One side: the name box (a drop-down when club is not 0, else a link to the profile), the line under it
 * and the rows. */
static void side(const Sides *s, const Player *me, const Player *other, int left, int x1, int y1, int x2, int y2,
                 uint32_t club, uint32_t id, int page)
{
    panel_at(x1, y1, x2, y2);
    uint32_t colour = left ? BLUE : ORANGE;
    if (club) chooser(x1 + 4, y1 + 4, x2 - 4, y1 + 4 + ROW_PX, colour, me->name, club, id, page);
    else text_at(x1 + 4, y1 + 4, x2 - 4, y1 + 4 + ROW_PX, 0x30, colour, DARK, 0xc, me->name, PROFILE_ID, me->staff);
    text_at(x1 + 4, y1 + 6 + ROW_PX, x2 - 4, y1 + 6 + 2 * ROW_PX, 0, 0, rd16(g_mem, LIST_TEXT), 0xc, me->line, 0, 0);
    int top = y1 + 10 + 2 * ROW_PX, col_w = s->name_w + s->value_w;
    for (int c = 0; c < s->columns; c++) {
        int x = x1 + 4 + c * (col_w + GAP_PX), y = top;
        for (int i = 0; i < s->k; i++) {
            if ((int)((s->second >> i) & 1) != c && s->columns == 2) continue;
            for (int r = s->g[i].start; r < s->g[i].start + s->g[i].count; r++, y += ROW_PX) {
                int k = s->rows[r];
                if (groups_heading(k)) {
                    text_at(x, y, x + col_w, y + ROW_PX - 1, 0, 0, rd16(g_mem, ITEM_HIGHLIGHT), 1, s->names[r], 0, 0);
                    continue;
                }
                text_at(x, y, x + s->name_w - 1, y + ROW_PX - 1, 0, 0, rd16(g_mem, LIST_TEXT), 1, s->names[r], 0, 0);
                char v[8];
                int mine = me->attr[k], theirs = other->attr[k];
                snprintf(v, sizeof v, mine ? "%d" : "-", mine);
                int w = text_at(x + s->name_w, y, x + col_w - 1, y + ROW_PX - 2, 0, 0,
                                mine ? columns_attribute_colour(mine) : rd16(g_mem, LIST_TEXT), 0xc, v, 0, 0);
                if (mine && theirs && mine > theirs && shown.marks < MARKS) shown.mark[shown.marks++] = w;
            }
        }
    }
}

/* The polygon's box with the axis names round it, and the table of the axis values under it. */
static void middle(const Sides *s, const Player *p, int x1, int y1, int x2, int y2)
{
    panel_at(x1, y1, x2, y2);
    /* A dark fill over the whole middle panel, so the background picture does not show through behind
     * the polygon (Q158). Its widget comes first, so the game draws the texts over it. */
    shown.backdrop = text_at(x1 + 1, y1 + 1, x2 - 1, y2 - 1, 0, 0, 0, 0xc, "", 0, 0);
    const CompareAxis *axes = compare_axes(s->keepers);
    int label_w = 0;
    for (int i = 0; i < COMPARE_AXES; i++) {
        int w = width_of(axes[i].name);
        if (w > label_w) label_w = w;
        shown.value[0][i] = compare_axis_value(&axes[i], p[0].attr);
        shown.value[1][i] = compare_axis_value(&axes[i], p[1].attr);
    }
    label_w += 6;

    /* The table: a heading row with the names in the players' colours (the legend), then one row for
     * each axis. */
    int table_h = (COMPARE_AXES + 1) * ROW_PX + 4, ty = y2 - 4 - table_h;
    char n1[64], n2[64];
    short_name(p[0].staff, n1, sizeof n1);
    short_name(p[1].staff, n2, sizeof n2);
    int value_w = width_of(n1), w2 = width_of(n2);
    if (w2 > value_w) value_w = w2;
    value_w += ROW_PX + 10;
    if (value_w < 60) value_w = 60;
    int table_w = label_w + 2 * value_w, tx = (x1 + x2 - table_w) / 2;
    if (tx < x1 + 4) tx = x1 + 4;
    for (int c = 0; c < 2; c++) {
        int vx = tx + label_w + c * value_w;
        text_at(vx + 4, ty + 3, vx + ROW_PX - 4, ty + ROW_PX - 4, 0x30, c ? ORANGE : BLUE, 0, 0xc, "", 0, 0);
        text_at(vx + ROW_PX, ty, vx + value_w - 1, ty + ROW_PX - 1, 0, 0, c ? ORANGE : BLUE, 1, c ? n2 : n1, 0, 0);
    }
    for (int i = 0; i < COMPARE_AXES; i++) {
        int y = ty + (i + 1) * ROW_PX;
        text_at(tx, y, tx + label_w - 1, y + ROW_PX - 1, 0, 0, rd16(g_mem, ITEM_HIGHLIGHT), 1, axes[i].name, 0, 0);
        for (int c = 0; c < 2; c++) {
            char v[16];
            compare_value_text(shown.value[c][i], v, sizeof v);
            int vx = tx + label_w + c * value_w + ROW_PX / 2, mine = shown.value[c][i], theirs = shown.value[!c][i];
            int w = text_at(vx, y, vx + 40, y + ROW_PX - 2, 0, 0, c ? ORANGE : BLUE, 0xc, v, 0, 0);
            if (mine >= 0 && theirs >= 0 && mine > theirs && shown.marks < MARKS) shown.mark[shown.marks++] = w;
        }
    }

    /* The polygon: the largest that leaves room for the axis names round it. When the names leave a
     * radius under SMALL_PX (classic mode), the polygon gets the short names; the table keeps the names. */
    int short_w = 0;
    for (int i = 0; i < COMPARE_AXES; i++) {
        int w = width_of(axes[i].abbr);
        if (w > short_w) short_w = w;
    }
    short_w += 6;
    int py1 = y1 + 4, py2 = ty - 6, cx = (x1 + x2) / 2, cy = (py1 + py2) / 2;
    int r = (x2 - x1 - 8) / 2 - label_w - 6, rv = (py2 - py1) / 2 - ROW_PX - 4, abbr = 0;
    if (r < SMALL_PX) {
        abbr = 1;
        label_w = short_w;
        r = (x2 - x1 - 8) / 2 - label_w - 6;
    }
    if (rv < r) r = rv;
    if (r < 20) r = 20;
    shown.radius = r;
    shown.polygon = text_at(cx - r, cy - r, cx + r, cy + r, 0, 0, 0, 0xc, "", 0, 0);
    for (int i = 0; i < COMPARE_AXES; i++) {
        int lx, ly;
        compare_corner(i, 200, cx, cy, r + 6, &lx, &ly);
        int lx1, lx2, flags;
        if (lx > cx + 4) { lx1 = lx; lx2 = lx + label_w; flags = 1; }
        else if (lx < cx - 4) { lx1 = lx - label_w; lx2 = lx; flags = 0x40; }
        else { lx1 = lx - label_w / 2; lx2 = lx + label_w / 2; flags = 0xc; }
        int ly1 = ly - ROW_PX / 2;
        if (ly < cy - r + 4) ly1 = ly - ROW_PX;
        else if (ly > cy + r - 4) ly1 = ly;
        text_at(lx1, ly1, lx2, ly1 + ROW_PX - 1, 0, 0, rd16(g_mem, ITEM_HIGHLIGHT), flags,
                abbr ? axes[i].abbr : axes[i].name, 0, 0);
    }
}

void compare_build(uint32_t text, uint32_t staff1, uint32_t staff2, int x1, int y1, int x2, int y2, uint32_t club,
                   int page1, int page2)
{
    buf = text;
    memset(&shown, 0, sizeof shown);
    shown.polygon = -1;
    shown.backdrop = -1;
    uint32_t node = rd32(g_mem, TOP_NODE);
    shown.build = node ? rd32(g_mem, node) : 0;
    Player p[2];
    load(&p[0], staff1);
    load(&p[1], staff2);

    static Sides s;
    s.keepers = p[0].keeper && p[1].keeper;
    s.n = compare_rows(s.keepers, s.rows, COMPARE_ROWS);
    s.k = groups_of(s.rows, s.n, s.g);
    s.name_w = 0;
    for (int r = 0; r < s.n; r++) {
        const char *h = groups_heading(s.rows[r]);
        if (h) snprintf(s.names[r], sizeof s.names[r], "%s", h);
        else attribute_name(s.rows[r], s.names[r], sizeof s.names[r]);
        int w = width_of(s.names[r]);
        if (w > s.name_w) s.name_w = w;
    }
    s.name_w += 8;
    s.value_w = width_of("20") + 14;

    int X1 = sx(x1), Y1 = sy(y1), X2 = sx(x2 + 1) - 1, Y2 = sy(y2 + 1) - 1;
    int fit = (Y2 - Y1 - 14 - 2 * ROW_PX) / ROW_PX;
    s.columns = s.n <= fit ? 1 : 2;
    s.second = s.columns == 2 ? two_columns(s.g, s.k) : 0;
    int w = side_width(&s);

    side(&s, &p[0], &p[1], 1, X1, Y1, X1 + w, Y2, club, CHOOSE1_ID, page1);
    side(&s, &p[1], &p[0], 0, X2 - w, Y1, X2, Y2, club, CHOOSE2_ID, page2);
    middle(&s, p, X1 + w + 6, Y1, X2 - w - 6, Y2);

    if (log_on()) {
        fprintf(stderr, "recomp: compare staff %d %d keepers %d columns %d rows %d", p[0].staff ?
                (int32_t)rd32(g_mem, p[0].staff) : -1, p[1].staff ? (int32_t)rd32(g_mem, p[1].staff) : -1,
                s.keepers, s.columns, s.n);
        for (int c = 0; c < 2; c++) {
            fprintf(stderr, " attr%d", c + 1);
            for (int k = 1; k <= 31; k++) fprintf(stderr, " %d", p[c].attr[k]);
        }
        for (int c = 0; c < 2; c++) {
            fprintf(stderr, " axes%d", c + 1);
            for (int i = 0; i < COMPARE_AXES; i++) fprintf(stderr, " %d", shown.value[c][i]);
        }
        fprintf(stderr, " marks %d\n", shown.marks);
    }
}

/* A click on a drop-down item of the Compare tab. A player: returns the side (1 or 2) with the staff
 * record in *value. A page item: returns 3 or 4 (side 1 or 2) with the page in *value. Else 0. */
int compare_input(uint32_t club, uint32_t id, uint32_t data, uint32_t *value)
{
    if (id != CHOOSE1_ID && id != CHOOSE2_ID) return 0;
    int side = id == CHOOSE1_ID ? 1 : 2;
    if (data >= PAGE_ITEM) {
        *value = data - PAGE_ITEM;
        return side + 2;
    }
    if (data < 1 || data > 50) return 0;
    *value = rd32(g_mem, club + SQUAD + 4 * (data - 1));
    return *value ? side : 0;
}

/* The first two players of the club's squad list, for a Compare tab with no choice yet. */
void compare_first_two(uint32_t club, uint32_t *a, uint32_t *b)
{
    *a = *b = 0;
    for (uint32_t i = 0; i < 50; i++) {
        uint32_t staff = rd32(g_mem, club + SQUAD + 4 * i);
        if (!staff || !rd32(g_mem, staff + PLAYER_DATA)) continue;
        if (!*a) *a = staff;
        else if (!*b) { *b = staff; return; }
    }
}

static void frame(const DrawTarget *t, int x1, int y1, int x2, int y2, uint16_t colour)
{
    draw_line(t, x1, y1, x2, y1, colour);
    draw_line(t, x2, y1, x2, y2, colour);
    draw_line(t, x2, y2, x1, y2, colour);
    draw_line(t, x1, y2, x1, y1, colour);
}

static void shape(const DrawTarget *t, int cx, int cy, int r, const int *value, uint16_t colour)
{
    int xy[2 * COMPARE_AXES];
    for (int i = 0; i < COMPARE_AXES; i++) compare_corner(i, value[i], cx, cy, r, &xy[2 * i], &xy[2 * i + 1]);
    draw_polygon(t, xy, COMPARE_AXES, colour, FILL_ALPHA);
    for (int i = 0; i < COMPARE_AXES; i++) {
        int j = (i + 1) % COMPARE_AXES;
        for (int d = 0; d < 2; d++) {       /* a line two pixels wide */
            draw_line(t, xy[2 * i] + d, xy[2 * i + 1], xy[2 * j] + d, xy[2 * j + 1], colour);
            draw_line(t, xy[2 * i], xy[2 * i + 1] + d, xy[2 * j], xy[2 * j + 1] + d, colour);
        }
        int dot[] = {xy[2 * i] - 2, xy[2 * i + 1] - 2, xy[2 * i] + 3, xy[2 * i + 1] - 2, xy[2 * i] + 3,
                     xy[2 * i + 1] + 3, xy[2 * i] - 2, xy[2 * i + 1] + 3};
        draw_polygon(t, dot, 4, colour, 256);
    }
}

/* After the game draws widget w: the polygon on its widget, a frame round each marked number. */
void compare_widget_drawn(uint32_t w)
{
    if (!shown.build || rd32(g_mem, NO_DRAW)) return;
    uint32_t node = rd32(g_mem, TOP_NODE);
    if (!node || rd32(g_mem, node) != shown.build) return;
    uint32_t first = display() + WIDGETS;
    if (w < first || (w - first) % WIDGET_SIZE) return;
    int i = (int)((w - first) / WIDGET_SIZE), mark = 0;
    for (int m = 0; m < shown.marks; m++) mark |= shown.mark[m] == i;
    if (i != shown.polygon && i != shown.backdrop && !mark) return;
    uint32_t surface = rd32(g_mem, DRAW_SURFACE);
    if (!surface) return;
    DrawTarget t = {g_mem + rd32(g_mem, surface + 0x14), rd32(g_mem, surface + 0x10),
                    (int)rd32(g_mem, surface + 8), (int)rd32(g_mem, surface + 0xc)};
    int x1 = (int32_t)rd32(g_mem, w + 0x10), y1 = (int32_t)rd32(g_mem, w + 0x14);
    int x2 = (int32_t)rd32(g_mem, w + 0x18), y2 = (int32_t)rd32(g_mem, w + 0x1c);
    if (i == shown.backdrop) {
        int box[] = {x1, y1, x2 + 1, y1, x2 + 1, y2 + 1, x1, y2 + 1};
        draw_polygon(&t, box, 4, BACKDROP, BACKDROP_ALPHA);
        return;
    }
    if (mark) {
        frame(&t, x1, y1, x2, y2, WHITE);
        return;
    }
    int cx = (x1 + x2) / 2, cy = (y1 + y2) / 2, r = shown.radius;
    for (int ring = 50; ring <= 200; ring += 50)
        for (int a = 0; a < COMPARE_AXES; a++) {
            int ax, ay, bx, by;
            compare_corner(a, ring, cx, cy, r, &ax, &ay);
            compare_corner((a + 1) % COMPARE_AXES, ring, cx, cy, r, &bx, &by);
            draw_line(&t, ax, ay, bx, by, GRID);
        }
    for (int a = 0; a < COMPARE_AXES; a++) {
        int ax, ay;
        compare_corner(a, 200, cx, cy, r, &ax, &ay);
        draw_line(&t, cx, cy, ax, ay, GRID);
    }
    shape(&t, cx, cy, r, shown.value[0], BLUE);
    shape(&t, cx, cy, r, shown.value[1], ORANGE);
}
