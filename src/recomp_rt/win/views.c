/* The Team views screen (spec 2026-10-15-features-0.4.0, slice 3; #22).
 *
 * A new screen of the runtime on the game's own screen stack. The "Team views" item of the Assistant
 * drop-down on a squad screen (assistant.c) calls views_open, which pushes the screen with the push of
 * the original 0x7fe600, flag 0, as the club screen does (0x453e2c). Its build and input functions are
 * host functions at fake addresses (rt_host_fn); the main loop 0x7fc7e0 calls them as it calls the
 * original's. So Back, the side bar and the history work as on any screen of the original.
 *
 * The frame copies the club screen 0x454900 (docs/research/2026-10-15-team-views-frame-inventory.md):
 * the club's background picture, the heading with the club's name in its colours, the side bar, a tab
 * bar with Best XI, Squad depth and Compare players, and the Back/Next bar. The tabs hold empty panels
 * in this slice; slices 4 to 6 fill them. The Best XI tab draws a pitch (draw.c) over a panel widget,
 * after the game draws that widget (views_widget_drawn, from the widget draw wrap in names.c), and the
 * eleven over it (best_xi.c). The Squad depth tab is in squad_depth.c.
 *
 * Node slots of the screen: 0 club, 1 team (0 = first team), 2 the chosen tactic of Best XI and Squad
 * depth, 4 tab (1 to 3), 5 and 6 the heading colours. Only numbers, so the screen needs no leave function
 * (inventory section 5).
 *
 * The screen only reads the game. It never changes the team or any other data. */
#include "win.h"
#include "draw.h"
#include "views.h"
#include "options.h"

#include <stdio.h>
#include <string.h>

#define SCREENS 0xb74450u           /* the screen stack S */
#define TOP_NODE (SCREENS + 0x7d4u + 0x0eu)
#define PUSH 0x7fe600u              /* thiscall on S (build, input, flag, leave, exit), ret 0x14 */
#define SET_SLOT 0x7ff270u          /* thiscall on S (slot, value, copy) */
#define NODE_SLOT 0x7ff010u         /* thiscall on S (slot) */
#define CANNOT_GO_BACK 0x7fec60u    /* thiscall on S (any): 0 when there is a screen to go back to */
#define AT_END 0x7febb0u            /* thiscall on S: 1 when no screen comes after the current one */
#define BACKGROUND 0x41c250u        /* cdecl (club, 0, 0): the club's background picture */
#define CLUB_COLOURS 0x53b640u      /* cdecl (club, kit, u16 *text colour, u16 *bar colour) */
#define CLUB_NAME 0x542100u         /* cdecl (club, team, buffer, size, mode, flag); mode 3: full name */
#define HEADING 0x803f30u           /* thiscall on S (bar colour, text colour, text) */
#define SIDE_BAR 0x75db00u          /* cdecl (mode, arg) */
#define TAB_BAR 0x5ed780u           /* cdecl (entries, count, selected, int *y_top, int *y_bottom, split) */
#define BACK_BAR 0x5edcc0u          /* cdecl (back, next) */
#define AREA_CREATE 0x55fba0u       /* thiscall on a Display, 11 args */
#define WIDGET_CREATE 0x55f990u     /* thiscall on a Display, 18 args */
#define SIDE_BAR_INPUT 0x7615d0u    /* cdecl (widget): 0 when it does not handle the click */
#define GAME_INPUT 0x7643d0u        /* cdecl (widget): the game's own actions, Back among them */

#define DISPLAYS 0xb74c4cu          /* the Display of a player slot, 0x300 bytes apart */
#define PLAYER_SLOT 0xb77c7au       /* s16: the player slot that has the turn */
#define WIDGETS 0xba95eu            /* Display + this: widget 0; 0x18c bytes each */
#define WIDGET_SIZE 0x18cu
#define NO_DRAW 0xaebe0cu           /* not 0: the draw functions of the original draw nothing */
#define DRAW_SURFACE 0xaebde4u      /* the surface that the draw functions of the original draw on */

#define TAB_ID 0x61u                /* the tabs; data 1 to 3. No shared input code uses 0x61. */
#define TAB_ENTRY 0xd4u
#define TABS 3
#define REBUILD 0xfffffff5u         /* -0xb */

static uint32_t scratch;            /* a block of the game's heap for the calls that write through pointers */
#define TEXT_AT (TABS * TAB_ENTRY + 0x200u)   /* the text buffer of the Best XI tab */
#define SCRATCH_SIZE (TEXT_AT + VIEWS_TEXT_SIZE)

static int pitch_widget = -1;       /* the panel that the pitch covers, on the screen built last */

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

/* A panel in the style of the squad list's background: an area with flags 2 (dark) and one cell. */
static void panel(int x1, int y1, int x2, int y2)
{
    uint32_t a[11] = {(uint32_t)x1, (uint32_t)y1, (uint32_t)x2, (uint32_t)y2, 1, 0, 1, 0, 2, 0, 0xffffffffu};
    call_this(AREA_CREATE, display(), 11, a);
}

/* A widget with no panel, no text and no action, for a picture of the runtime; returns its index. */
static int plain_widget(int x1, int y1, int x2, int y2)
{
    uint32_t empty = scratch + 0x1f0;
    wr8(g_mem, empty, 0);
    uint32_t w[18] = {2, (uint32_t)x1, (uint32_t)y1, (uint32_t)x2, (uint32_t)y2, 0, 0, 0, 0, 0, 0, 1, 0, empty, 0,
                      0, 0, 0xffffffffu};
    return (int16_t)call_this(WIDGET_CREATE, display(), 18, w);
}

static void heading(uint32_t club, uint32_t team)
{
    uint32_t text = scratch, colours = scratch + 0x180;
    uint32_t bar = node_slot(6), fg = node_slot(5);
    if (!bar && !fg) {
        wr32(g_mem, colours, 0);
        wr32(g_mem, colours + 4, 0);
        uint32_t c[4] = {club, 1, colours, colours + 4};
        call(CLUB_COLOURS, 4, c);
        fg = rd16(g_mem, colours);
        bar = rd16(g_mem, colours + 4);
        set_slot(5, fg);
        set_slot(6, bar);
    }
    wr8(g_mem, text, 0);
    uint32_t n[6] = {club, team, text, 0x12c, 3, 1};
    call(CLUB_NAME, 6, n);
    uint32_t h[3] = {bar, fg, text};
    call_this(HEADING, SCREENS, 3, h);
}

/* Returns the y below the tab row. */
static int tab_bar(int selected)
{
    static const char *const names[TABS] = {"Best XI", "Squad depth", "Compare players"};
    uint32_t e = scratch, y = scratch + TABS * TAB_ENTRY;
    for (uint32_t i = 0; i < TABS; i++) {
        uint32_t at = e + i * TAB_ENTRY;
        memset(g_mem + at, 0, TAB_ENTRY);
        wr16(g_mem, at, 0xffff);                     /* font -1: font 1 */
        strcpy(win_str(at + 4), names[i]);
        wr32(g_mem, at + 0xcc, TAB_ID);
        wr32(g_mem, at + 0xd0, i + 1);
    }
    wr32(g_mem, y, 0x50);
    wr32(g_mem, y + 4, 0x221);
    uint32_t a[6] = {e, TABS, (uint32_t)selected, y, y + 4, 1};
    call(TAB_BAR, 6, a);
    return (int32_t)rd32(g_mem, y);
}

/* The build of the screen: cdecl, no arguments. */
static void views_build(void)
{
    if (!scratch) scratch = rt_alloc_zero(SCRATCH_SIZE);
    uint32_t club = node_slot(0), team = node_slot(1);
    int tab = (int)node_slot(4);
    if (tab < 1 || tab > TABS) {
        tab = 1;
        set_slot(4, 1);
    }
    uint32_t b[3] = {club, 0, 0};
    call(BACKGROUND, 3, b);
    heading(club, team);
    uint32_t s[2] = {4, 0};
    call(SIDE_BAR, 2, s);
    int top = tab_bar(tab), bottom = 545;

    pitch_widget = -1;
    if (tab == 1) {
        /* Best XI: the pitch on the left, the table on the right. The players' widgets come after the
         * pitch widget, so the game draws them over the pitch. */
        panel(100, top, 440, bottom);
        int x1 = 104, y1 = top + 8, x2 = 436, y2 = bottom - 8;
        pitch_widget = plain_widget(x1, y1, x2, y2);
        views_pitch_rect(&x1, &y1, &x2, &y2);
        panel(450, top, 790, bottom);
        best_xi_build(club, team, scratch + TEXT_AT, x1, y1, x2, y2, 460, top + 6, 780, bottom - 6, node_slot(6),
                      node_slot(5));
    } else if (tab == 2) {
        panel(100, top, 790, bottom);
        squad_depth_build(club, team, scratch + TEXT_AT, 110, top + 6, 780, bottom - 6, node_slot(6));
    } else {
        panel(100, top, 790, bottom);
    }

    uint32_t back_arg[1] = {0};
    uint32_t back = call_this(CANNOT_GO_BACK, SCREENS, 1, back_arg) == 0;
    uint32_t next = call_this(AT_END, SCREENS, 0, NULL) == 0;
    uint32_t bar[2] = {back, next};
    call(BACK_BAR, 2, bar);
    fprintf(stderr, "recomp: team views built, tab %d\n", tab);
    rt_ret_cdecl(0);
}

/* The input of the screen: cdecl (s16 widget), returns the result for the main loop. A tab click
 * changes slot 4 and rebuilds, as the club input does for its tabs (0x467701). */
static void views_input(void)
{
    int16_t w = (int16_t)rt_arg(0);
    if (w >= 0) {
        uint32_t at = display() + WIDGETS + 8 + (uint32_t)w * WIDGET_SIZE;
        if (best_xi_input(rd32(g_mem, at), rd32(g_mem, at + 0x40))) {
            rt_ret_cdecl(REBUILD);
            return;
        }
        if ((int16_t)rd32(g_mem, at) == (int16_t)TAB_ID) {
            uint32_t data = rd32(g_mem, at + 0x40);
            if (data != node_slot(4)) {
                set_slot(4, data);
                rt_ret_cdecl(REBUILD);
                return;
            }
        }
    }
    uint32_t a[1] = {rt_arg(0)};
    uint32_t r = call(SIDE_BAR_INPUT, 1, a);
    if (!r) r = call(GAME_INPUT, 1, a);
    rt_ret_cdecl(r);
}

static uint32_t build_fn(void) { return rt_host_fn("views_build", views_build); }

static int on_top(void)
{
    uint32_t node = rd32(g_mem, TOP_NODE);
    return node && rd32(g_mem, node) == build_fn();
}

/* The largest pitch of 68 by 105 that fits the box (screen pixels), in its middle, but at least as wide as
 * five name boxes and their gaps (Q154) when the box allows: then the pitch is wider than 68 by 105. */
#define MIN_PITCH_W (5 * (VIEWS_BOX_PX + 4) + 4)
static void fit_pitch(int *x1, int *y1, int *x2, int *y2)
{
    int bw = *x2 - *x1, bh = *y2 - *y1;
    if (bw * 105 > bh * 68) {
        int pw = bh * 68 / 105;
        if (pw < MIN_PITCH_W) pw = MIN_PITCH_W < bw ? MIN_PITCH_W : bw;
        *x1 += (bw - pw) / 2;
        *x2 = *x1 + pw;
    } else {
        int ph = bw * 105 / 68;
        *y1 += (bh - ph) / 2;
        *y2 = *y1 + ph;
    }
}

/* The pitch of a box in classic numbers, back in classic numbers: the box scales to the screen as the
 * widget create does (display.c), the pitch fits it there, and its edges scale back. */
void views_pitch_rect(int *x1, int *y1, int *x2, int *y2)
{
    int w = (int)rt_screen_w, h = (int)rt_screen_h;
    int sx1 = *x1 * w / 800, sy1 = *y1 * h / 600, sx2 = (*x2 + 1) * w / 800 - 1, sy2 = (*y2 + 1) * h / 600 - 1;
    fit_pitch(&sx1, &sy1, &sx2, &sy2);
    *x1 = sx1 * 800 / w;
    *y1 = sy1 * 600 / h;
    *x2 = sx2 * 800 / w;
    *y2 = sy2 * 600 / h;
}

/* The widget draw wrap (names.c) calls this after the game draws widget w (an address). The pitch goes
 * on the game's draw surface inside the panel's box, so the screen dumps hold it too. */
void views_widget_drawn(uint32_t w)
{
    if (pitch_widget < 0 || !on_top() || rd32(g_mem, NO_DRAW)) return;
    if (w != display() + WIDGETS + (uint32_t)pitch_widget * WIDGET_SIZE) return;
    uint32_t surface = rd32(g_mem, DRAW_SURFACE);
    if (!surface) return;
    DrawTarget t = {g_mem + rd32(g_mem, surface + 0x14), rd32(g_mem, surface + 0x10),
                    (int)rd32(g_mem, surface + 8), (int)rd32(g_mem, surface + 0xc)};
    int x1 = (int32_t)rd32(g_mem, w + 0x10), y1 = (int32_t)rd32(g_mem, w + 0x14);
    int x2 = (int32_t)rd32(g_mem, w + 0x18), y2 = (int32_t)rd32(g_mem, w + 0x1c);
    fit_pitch(&x1, &y1, &x2, &y2);
    draw_pitch(&t, x1, y1, x2, y2);
}

void views_open(uint32_t club, uint32_t team)
{
    uint32_t p[5] = {build_fn(), rt_host_fn("views_input", views_input), 0, 0, 0};
    call_this(PUSH, SCREENS, 5, p);
    set_slot(0, club);
    set_slot(1, team);
    set_slot(4, 1);
    fprintf(stderr, "recomp: team views for club %#x team %u\n", club, team);
}
