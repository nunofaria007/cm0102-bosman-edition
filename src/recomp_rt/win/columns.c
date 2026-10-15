/* Attribute colours (#19) and match columns (#23).
 *
 * Attribute colours: the label create 0x804100 makes every attribute number. The runtime knows the two
 * call sites by their return address (tools/recomp/callsites.py): the value cells of the player profile
 * and the Attributes view of the squad list. For a text of 1 to 20 it changes the text colour argument to
 * the colour of the value band. All other labels, and every label with the option off, keep the colours
 * of the original.
 *
 * Match columns, in widescreen mode only (the classic screen has no room; RECOMP_COLUMNS_CLASSIC=1 makes
 * a test run show them in classic mode too):
 * - The tactics screen 0x899ea0 in a match, view Match Rating (node slot 10 = 6), shows the player's
 *   positions and condition beside the match rating. Its player list area gets two more columns. The
 *   original cells keep their order: the runtime moves their column numbers, and after the screen code
 *   it adds a position label and a condition label to each row, with the game's own getters (0x8ab480
 *   and 0x53ee50 as in the Position(s) view, 0x8aafd0 as in the Condition view).
 * - The Player Ratings view 0x722ed0 of the match screen shows each player's condition beside the
 *   rating, for both teams. Its area gets one more column per team. The labels come after the screen
 *   code, so that the widget numbers that 0x723350 uses to update the view do not move. 0x723350 runs
 *   each time the view updates, and the runtime then updates the conditions too.
 *
 * Both features only read the game's data and add widgets on the screen, so they change no game state.
 * See docs/research/2026-10-14-columns-and-colours-inventory.md. */
#include "win.h"
#include "options.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

extern const uint32_t rt_site_profile_value;
extern const uint32_t rt_site_squad_attribute;
extern const uint32_t rt_site_tactics_area;
extern const uint32_t rt_site_tactics_area_heading;
extern const uint32_t rt_site_tactics_status;
extern const uint32_t rt_site_ratings_area;

/* The colours of the bands, RGB565, light enough for the dark backgrounds of the game. */
#define COLOUR_LOW 0xFA8Au       /* 1 to 5: red (255, 80, 80) */
#define COLOUR_MID 0xFD84u       /* 6 to 10: amber (255, 176, 32) */
#define COLOUR_GOOD 0xA70Cu      /* 11 to 15: light green (160, 224, 96) */
#define COLOUR_TOP 0x37E6u       /* 16 to 20: bright green (48, 255, 48) */

#define SCREENS 0xb74450u        /* the screen stack, this of 0x804100, 0x801c70, 0x7ff010 */
#define LABEL_CREATE 0x804100u   /* thiscall (area, column, row, style, colour, text_flags, font, text_colour,
                                  * text): the widget number in ax */
#define SET_TEXT 0x801c70u       /* thiscall (widget, text) */
#define NODE_SLOT 0x7ff010u      /* thiscall (slot): the value of a slot of the current screen node */
#define WIDGETS 0xb77c94u + 0xba95eu   /* the widgets of screen manager 0 (names.c) */
#define WIDGET_SIZE 0x18cu
#define MAX_WIDGETS 1200
#define YELLOW 0xaebdecu         /* the colour globals of the original */
#define GREY 0xae317eu
#define PERSONS 0xae23bcu        /* pointer to the person records, 0x6e bytes each */
#define MATCH 0xb67a64u          /* pointer to the match being played: the object at +4 of the tactics
                                  * object (0x8aafd0), found at this address in a run */

/* Scratch text and the column weights: the end of the game's text buffer 0xde1f64 (2000 bytes or more,
 * 0x53ee50 gets 2000 as its size). Screen code only writes short texts at its start. The runtime keeps
 * the bytes it uses and puts them back. */
#define SCRATCH (0xde1f64u + 1600u)
#define SCRATCH_SIZE 256u

/* The attribute colour of a text that is a whole number 1 to 20, or 0 for any other text. */
static uint32_t attribute_colour(const char *s)
{
    int v = 0, n = 0;
    while (*s == ' ') s++;
    for (; *s >= '0' && *s <= '9' && n < 3; s++, n++) v = v * 10 + (*s - '0');
    while (*s == ' ') s++;
    if (!n || *s || v < 1 || v > 20) return 0;
    return v <= 5 ? COLOUR_LOW : v <= 10 ? COLOUR_MID : v <= 15 ? COLOUR_GOOD : COLOUR_TOP;
}

/* Calls of the game's functions. Each keeps the registers of the caller. */
static uint32_t call_this(uint32_t fn, uint32_t self, int n, const uint32_t *args)
{
    Cpu saved = cpu;
    cpu.ecx = self;
    uint32_t r = rt_callback(fn, n, args);
    cpu = saved;
    return r;
}

static uint32_t call_cdecl(uint32_t fn, int n, const uint32_t *args)
{
    Cpu saved = cpu;
    uint32_t r = rt_callback(fn, n, args);
    cpu = saved;
    return r;
}

static uint32_t node_slot(int n)
{
    uint32_t a[1] = {(uint32_t)n};
    return call_this(NODE_SLOT, SCREENS, 1, a);
}

static int label(int area, int column, int row, uint32_t text_flags, uint32_t font, uint32_t colour, const char *text)
{
    win_put_str(SCRATCH, text, SCRATCH_SIZE);
    uint32_t a[9] = {(uint32_t)area, (uint32_t)column, (uint32_t)row, 1, 0, text_flags, font, colour, SCRATCH};
    return (int16_t)call_this(LABEL_CREATE, SCREENS, 9, a);
}

static int columns_wanted(void)
{
    static int classic = -1;
    if (classic < 0) {
        const char *v = getenv("RECOMP_COLUMNS_CLASSIC");
        classic = v && *v == '1';
    }
    return options_bool(OPT_MATCH_COLUMNS) && (rt_screen_w > 800u || classic);
}

/* --- Column moves: while a screen builds, the widgets of one area get new column numbers. --- */

static struct {
    int pending;             /* the next area create of the site gets the new columns */
    int area;                /* the area, or -1 */
    int columns;             /* the original's column count */
    const uint8_t *move;     /* new column of each original column */
    const uint8_t *weights;  /* the new weights */
    int new_columns;
} cols = {0, -1, 0, NULL, NULL, 0};

static uint8_t scratch_saved[SCRATCH_SIZE];
static void scratch_keep(void) { memcpy(scratch_saved, g_mem + SCRATCH, SCRATCH_SIZE); }
static void scratch_restore(void) { memcpy(g_mem + SCRATCH, scratch_saved, SCRATCH_SIZE); }

/* Tactics list, 9 columns: number, three boxes, name, the instruction or substitution column, status,
 * rating, goals. New: positions and condition after the status. */
static const uint8_t tactics_move[9] = {0, 1, 2, 3, 4, 5, 6, 9, 10};
static const uint8_t tactics_weights[11] = {14, 1, 5, 1, 52, 10, 14, 26, 13, 8, 7};
enum { TACTICS_POSITIONS = 7, TACTICS_CONDITION = 8 };

/* Player Ratings, 8 columns per team: number, box, card box, gap, name, event, rating, goals. New: the
 * condition before the rating. */
static const uint8_t ratings_move[16] = {0, 1, 2, 3, 4, 5, 7, 8, 9, 10, 11, 12, 13, 14, 16, 17};
static const uint8_t ratings_weights[18] = {14, 1, 5, 1, 60, 21, 12, 7, 7, 14, 1, 5, 1, 60, 21, 12, 7, 7};
enum { RATINGS_CONDITION = 6 };

/* Called by the area create wrap (display.c) before the original: the call sites of the two lists get
 * the new column count and weights. */
void columns_area_before(void)
{
    uint32_t ret = rd32(g_mem, cpu.esp);
    if (!cols.pending || (ret != rt_site_tactics_area && ret != rt_site_tactics_area_heading &&
                          ret != rt_site_ratings_area)) return;
    if ((int)rt_arg(4) != cols.columns) { cols.pending = 0; return; }
    scratch_keep();
    for (int i = 0; i < cols.new_columns; i++) wr8(g_mem, SCRATCH + (uint32_t)i, cols.weights[i]);
    wr32(g_mem, cpu.esp + 4 + 4 * 4, (uint32_t)cols.new_columns);
    wr32(g_mem, cpu.esp + 4 + 4 * 5, SCRATCH);
    cols.pending = 2;
}

/* Called by the area create wrap after the original, with the new area. */
void columns_area_after(int area)
{
    if (cols.pending != 2) return;
    scratch_restore();
    cols.pending = 0;
    cols.area = area;
}

/* Called by the widget create wrap (display.c) before the original: a widget of the moved area gets its
 * new column. Widget create: (flags, x1, y1, x2, y2, column, row, ..., area as argument 17). */
void columns_widget_before(void)
{
    if (cols.area < 0 || (int16_t)rt_arg(17) != cols.area) return;
    uint32_t c = rt_arg(5);
    if (c < (uint32_t)cols.columns) wr32(g_mem, cpu.esp + 4 + 4 * 5, cols.move[c]);
}

static void columns_off(void) { cols.pending = 0; cols.area = -1; }

/* --- The tactics screen. --- */

typedef struct { int row, index; uint32_t tactics; } TacticsRow;
static TacticsRow tactics_rows[24];
static int tactics_count, tactics_building;

void o_008aadb0(void);

/* The status text of a player, thiscall on the tactics object (index, text, size, colour out, live). The
 * call site in the Condition and Match Rating views gives the row of each player: esi index, ebp row. */
RT_WRAP(008aadb0, tactics_status)
{
    if (tactics_building && cols.area >= 0 && rd32(g_mem, cpu.esp) == rt_site_tactics_status &&
        tactics_count < 24) {
        TacticsRow *r = &tactics_rows[tactics_count++];
        r->row = (int)cpu.ebp;
        r->index = (int8_t)cpu.esi;
        r->tactics = cpu.ecx;
    }
    o_008aadb0();
}

/* The new labels already have their new columns: the column moves end before them. */
static void tactics_add_labels(int area)
{
    scratch_keep();
    for (int i = 0; i < tactics_count; i++) {
        TacticsRow *r = &tactics_rows[i];
        uint32_t a2[2] = {(uint32_t)r->index, 0}, a3[3] = {(uint32_t)r->index, 0, 0};
        uint32_t on_pitch = call_this(0x8aa9d0u, r->tactics, 3, a3);
        uint32_t colour = rd16(g_mem, on_pitch ? YELLOW : GREY);
        uint32_t player = call_this(0x8ab480u, r->tactics, 2, a2);
        if (player && rd32(g_mem, player + 0x61)) {
            wr8(g_mem, SCRATCH, 0);
            uint32_t p[4] = {rd32(g_mem, player + 0x61), SCRATCH, SCRATCH_SIZE, 1};
            call_cdecl(0x53ee50u, 4, p);
            char text[64];
            snprintf(text, sizeof text, "%s", win_str(SCRATCH));
            label(area, TACTICS_POSITIONS, r->row, 1, 1, colour, text);
        }
        int condition = (int8_t)call_this(0x8aafd0u, r->tactics, 2, a2);
        if (condition) {
            char text[16];
            if (condition < 1) snprintf(text, sizeof text, "Inj");
            else snprintf(text, sizeof text, "%d%%", condition);
            label(area, TACTICS_CONDITION, r->row, 0xc, 1, colour, text);
        }
    }
    scratch_restore();
}

void o_00899ea0(void);

/* The tactics screen build, a screen node function. */
RT_WRAP(00899ea0, tactics_build)
{
    columns_off();
    tactics_count = 0;
    if (columns_wanted() && (int8_t)node_slot(10) == 6) {
        cols.pending = 1;
        cols.columns = 9;
        cols.move = tactics_move;
        cols.weights = tactics_weights;
        cols.new_columns = 11;
    }
    tactics_building = 1;
    o_00899ea0();
    tactics_building = 0;
    int area = cols.area;
    columns_off();
    if (!rt_unwinding && area >= 0) tactics_add_labels(area);
}

/* --- The Player Ratings view. --- */

static struct { int area, widget[2][20]; uint32_t summary; } ratings = {-1, {{0}}, 0};

/* The condition of the player in a row of the view, in per cent, or -1. The view reads its rows from the
 * match summary (0x722ed0: summary + 0x190 + team * 0x625 + row * 0x49, person number at +0, 0xfb at
 * +0x1d for an empty row). The match keeps the condition of each player at +0x4d (a float, times 0.01
 * for per cent, 0x8aafd0) of its records at match + 0x4796, 0x1be bytes each, 20 per team, with
 * the person at +0x69 and 0xfb at +0x19 for an empty record. The runtime finds the player's record by
 * the person, so the rows of both teams line up whatever the order. */
static int ratings_condition(int team, int row)
{
    uint32_t rec = ratings.summary + 0x190u + (uint32_t)team * 0x625u + (uint32_t)row * 0x49u;
    if (rd8(g_mem, rec + 0x1d) == 0xfb) return -1;
    int32_t id = (int32_t)rd32(g_mem, rec);
    if (id < 0) return -1;      /* players of clubs outside the chosen leagues have a number past the
                                 * person count (0x7231bb), so there is no upper limit here */
    uint32_t person = rd32(g_mem, PERSONS) + (uint32_t)id * 0x6eu;
    uint32_t match = rd32(g_mem, MATCH);
    if (!match) return -1;
    for (uint32_t k = 0; k < 40; k++) {
        uint32_t m = match + 0x4796u + k * 0x1beu;
        if (rd8(g_mem, m + 0x19) == 0xfb || rd32(g_mem, m + 0x69) != person) continue;
        uint32_t bits = rd32(g_mem, m + 0x4d);
        float f;
        memcpy(&f, &bits, 4);
        double v = f * 0.01;
        return v < 0 ? 0 : (int)v;   /* the original truncates (__ftol) */
    }
    return -1;
}

static int widget_ok(int w, int column, int row)
{
    if (w < 0 || w >= MAX_WIDGETS) return 0;
    uint32_t a = WIDGETS + WIDGET_SIZE * (uint32_t)w;
    return (int16_t)rd16(g_mem, a + 0x7a) == ratings.area && (int32_t)rd32(g_mem, a + 0x20) == column &&
           (int32_t)rd32(g_mem, a + 0x24) == row;
}

static void ratings_refresh(void)
{
    if (ratings.area < 0) return;
    scratch_keep();
    for (int team = 0; team < 2; team++)
        for (int row = 0; row < 20; row++) {
            int w = ratings.widget[team][row];
            if (!widget_ok(w, team * 9 + RATINGS_CONDITION, row)) {
                ratings.area = -1;
                scratch_restore();
                return;
            }
            int c = ratings_condition(team, row);
            char text[16] = "";
            if (c >= 0) snprintf(text, sizeof text, "%d%%", c);
            win_put_str(SCRATCH, text, SCRATCH_SIZE);
            uint32_t a[2] = {(uint32_t)w, SCRATCH};
            call_this(SET_TEXT, SCREENS, 2, a);
        }
    scratch_restore();
}

void o_00722ed0(void);
void o_00723350(void);

/* The Player Ratings view build, cdecl (two pointers to the area rect). */
RT_WRAP(00722ed0, ratings_build)
{
    columns_off();
    ratings.area = -1;
    if (columns_wanted()) {
        cols.pending = 1;
        cols.columns = 16;
        cols.move = ratings_move;
        cols.weights = ratings_weights;
        cols.new_columns = 18;
    }
    o_00722ed0();
    int area = cols.area;
    columns_off();
    if (rt_unwinding || area < 0) return;
    uint32_t slot0 = node_slot(0);
    ratings.summary = slot0 ? rd32(g_mem, slot0 + 0x5f) : node_slot(0x20);
    scratch_keep();
    for (int team = 0; team < 2; team++)
        for (int row = 0; row < 20; row++)
            ratings.widget[team][row] = label(area, team * 9 + RATINGS_CONDITION, row, 0xc, 2, rd16(g_mem, YELLOW), "");
    scratch_restore();
    ratings.area = area;
    ratings_refresh();
}

/* The Player Ratings update: the runtime updates the conditions after it. */
RT_WRAP(00723350, ratings_update)
{
    o_00723350();
    if (!rt_unwinding) ratings_refresh();
}

void o_00804100(void);

/* The label create 0x804100. */
RT_WRAP(00804100, label_create)
{
    uint32_t ret = rd32(g_mem, cpu.esp);
    if ((ret == rt_site_profile_value || ret == rt_site_squad_attribute) && options_bool(OPT_ATTRIBUTE_COLOURS)) {
        uint32_t text = rt_arg(8);
        uint32_t colour = text ? attribute_colour(win_str(text)) : 0;
        if (colour) wr32(g_mem, cpu.esp + 4 + 4 * 7, colour);
    }
    o_00804100();
}
