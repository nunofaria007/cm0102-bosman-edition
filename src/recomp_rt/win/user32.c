/* USER32 over SDL: the window, the message queue fed from SDL events, the window procedure call
 * (DispatchMessageA calls the original's 0x68d630), cursors and the message box. Also the screen:
 * the SDL window shows the primary DirectDraw surface (ddraw.c) as an 800x600 RGB565 texture.
 *
 * For runs without a person: RECOMP_SCRIPT holds steps "<ms> <action> [args]" separated by ';', with
 * the time since the window opened. Actions: move x y, down x y, up x y, click x y (down, then up 100 ms
 * later), rclick x y, key <virtual key>, char <code>, type <text> (one character each 100 ms),
 * dump <file.bmp>, quit. RECOMP_DUMP names a file
 * to save the screen to when the program ends. */
#include "win.h"

#include <SDL.h>
#include <stdlib.h>
#include <strings.h>

#define WM_KEYDOWN 0x100u
#define WM_KEYUP 0x101u
#define WM_CHAR 0x102u
#define WM_MOUSEMOVE 0x200u
#define WM_LBUTTONDOWN 0x201u
#define WM_LBUTTONUP 0x202u
#define WM_RBUTTONDOWN 0x204u
#define WM_RBUTTONUP 0x205u
#define WM_MOUSEWHEEL 0x20Au
#define WM_QUIT 0x12u

uint32_t win_wndproc;
uint32_t win_hwnd = 0x00010010u;
#define DESKTOP_HWND 0x00010001u

static SDL_Window *window;
static SDL_Renderer *renderer;
static SDL_Texture *texture;
static uint64_t opened_at, last_present, last_poll;
extern int win_screen_dirty;
uint32_t win_primary_pixels(void);

/* The message queue. */
typedef struct { uint32_t msg, wparam, lparam, time; } Msg;
#define QUEUE 1024
static Msg queue[QUEUE];
static uint32_t q_head, q_tail;
static int mouse_x, mouse_y, buttons;

static void post(uint32_t msg, uint32_t wparam, uint32_t lparam)
{
    if (q_tail - q_head >= QUEUE) return;          /* full: drop, as a busy Windows queue would not */
    Msg *m = &queue[q_tail++ % QUEUE];
    m->msg = msg;
    m->wparam = wparam;
    m->lparam = lparam;
    m->time = (uint32_t)SDL_GetTicks();
}

static uint32_t mouse_lparam(void) { return ((uint32_t)mouse_y & 0xffff) << 16 | ((uint32_t)mouse_x & 0xffff); }

static void post_mouse(uint32_t msg, int x, int y)
{
    mouse_x = x;
    mouse_y = y;
    post(msg, (uint32_t)buttons, mouse_lparam());
}

/* The window. CM0102_VERSION comes from the CMake project version. */
#ifndef CM0102_VERSION
#define CM0102_VERSION "dev"
#endif
void win_video_init(void)
{
    if (window) return;
    window = SDL_CreateWindow("CM 01/02: Re-Signed Edition " CM0102_VERSION, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                              800, 600, SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI);
    if (!window) rt_fatal("SDL_CreateWindow: %s", SDL_GetError());
    renderer = SDL_CreateRenderer(window, -1, 0);
    if (!renderer) renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_SOFTWARE);
    if (!renderer) rt_fatal("SDL_CreateRenderer: %s", SDL_GetError());
    SDL_RenderSetLogicalSize(renderer, 800, 600);
    texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGB565, SDL_TEXTUREACCESS_STREAMING, 800, 600);
    if (!texture) rt_fatal("SDL_CreateTexture: %s", SDL_GetError());
    SDL_StartTextInput();
    opened_at = SDL_GetTicks64();
}

static void present_now(void)
{
    uint32_t px = win_primary_pixels();
    if (!renderer || !px) return;
    SDL_UpdateTexture(texture, NULL, g_mem + px, 1600);
    SDL_RenderClear(renderer);
    SDL_RenderCopy(renderer, texture, NULL, NULL);
    SDL_RenderPresent(renderer);
    win_screen_dirty = 0;
    last_present = SDL_GetTicks64();
}

/* At most about 60 times a second; the rest waits for the next poll. */
void win_present(void)
{
    win_screen_dirty = 1;
    if (SDL_GetTicks64() - last_present >= 16) present_now();
}

void win_dump_screen(const char *path)
{
    uint32_t px = win_primary_pixels();
    if (!px) { fprintf(stderr, "recomp: no screen to save to %s\n", path); return; }
    SDL_Surface *s = SDL_CreateRGBSurfaceWithFormatFrom(g_mem + px, 800, 600, 16, 1600, SDL_PIXELFORMAT_RGB565);
    SDL_Surface *rgb = s ? SDL_ConvertSurfaceFormat(s, SDL_PIXELFORMAT_RGB24, 0) : NULL;
    if (!rgb || SDL_SaveBMP(rgb, path)) fprintf(stderr, "recomp: cannot save %s: %s\n", path, SDL_GetError());
    else fprintf(stderr, "recomp: screen saved to %s\n", path);
    SDL_FreeSurface(rgb);
    SDL_FreeSurface(s);
}

/* Windows virtual keys of SDL keys. */
static uint32_t virtual_key(SDL_Keycode k)
{
    if (k >= SDLK_a && k <= SDLK_z) return 'A' + (uint32_t)(k - SDLK_a);
    if (k >= SDLK_0 && k <= SDLK_9) return '0' + (uint32_t)(k - SDLK_0);
    if (k >= SDLK_F1 && k <= SDLK_F12) return 0x70 + (uint32_t)(k - SDLK_F1);
    if (k >= SDLK_KP_1 && k <= SDLK_KP_9) return 0x61 + (uint32_t)(k - SDLK_KP_1);
    switch (k) {
    case SDLK_BACKSPACE: return 0x08;
    case SDLK_TAB: return 0x09;
    case SDLK_RETURN: case SDLK_KP_ENTER: return 0x0D;
    case SDLK_LSHIFT: case SDLK_RSHIFT: return 0x10;
    case SDLK_LCTRL: case SDLK_RCTRL: return 0x11;
    case SDLK_LALT: case SDLK_RALT: return 0x12;
    case SDLK_PAUSE: return 0x13;
    case SDLK_CAPSLOCK: return 0x14;
    case SDLK_ESCAPE: return 0x1B;
    case SDLK_SPACE: return 0x20;
    case SDLK_PAGEUP: return 0x21;
    case SDLK_PAGEDOWN: return 0x22;
    case SDLK_END: return 0x23;
    case SDLK_HOME: return 0x24;
    case SDLK_LEFT: return 0x25;
    case SDLK_UP: return 0x26;
    case SDLK_RIGHT: return 0x27;
    case SDLK_DOWN: return 0x28;
    case SDLK_INSERT: return 0x2D;
    case SDLK_DELETE: return 0x2E;
    case SDLK_KP_0: return 0x60;
    case SDLK_KP_MULTIPLY: return 0x6A;
    case SDLK_KP_PLUS: return 0x6B;
    case SDLK_KP_MINUS: return 0x6D;
    case SDLK_KP_PERIOD: return 0x6E;
    case SDLK_KP_DIVIDE: return 0x6F;
    case SDLK_SEMICOLON: return 0xBA;
    case SDLK_EQUALS: return 0xBB;
    case SDLK_COMMA: return 0xBC;
    case SDLK_MINUS: return 0xBD;
    case SDLK_PERIOD: return 0xBE;
    case SDLK_SLASH: return 0xBF;
    case SDLK_BACKQUOTE: return 0xC0;
    case SDLK_LEFTBRACKET: return 0xDB;
    case SDLK_BACKSLASH: return 0xDC;
    case SDLK_RIGHTBRACKET: return 0xDD;
    case SDLK_QUOTE: return 0xDE;
    default: return 0;
    }
}

/* The script of a run without a person. */
typedef struct { uint64_t at; char action[16]; int a, b; char text[512]; } Step;
static Step *steps;
static int step_count, step_next, script_loaded;

static void load_script(void)
{
    script_loaded = 1;
    const char *s = getenv("RECOMP_SCRIPT");
    if (!s) return;
    char *copy = strdup(s), *save = NULL;
    for (char *item = strtok_r(copy, ";", &save); item; item = strtok_r(NULL, ";", &save)) {
        Step st;
        memset(&st, 0, sizeof st);
        unsigned long long at = 0;
        int n = sscanf(item, " %llu %15s", &at, st.action);
        if (n < 2) continue;
        st.at = at;
        if (!strcmp(st.action, "dump") || !strcmp(st.action, "type")) sscanf(item, " %*u %*s %511s", st.text);
        else sscanf(item, " %*u %*s %d %d", &st.a, &st.b);
        if (!strcmp(st.action, "type")) {
            /* One character every 100 ms: the window procedure keeps only the last WM_CHAR. */
            size_t n = strlen(st.text);
            steps = realloc(steps, sizeof *steps * (size_t)(step_count + (int)n + 2));
            for (size_t i = 0; i < n; i++) {
                Step c = st;
                c.at = st.at + 100 * i;
                snprintf(c.action, sizeof c.action, "char");
                c.a = (unsigned char)st.text[i];
                c.text[0] = 0;
                steps[step_count++] = c;
            }
            continue;
        }
        steps = realloc(steps, sizeof *steps * (size_t)(step_count + 2));
        steps[step_count++] = st;
        if (!strcmp(st.action, "click") || !strcmp(st.action, "rclick")) {
            Step up = st;
            up.at += 100;
            snprintf(up.action, sizeof up.action, "%s", st.action[0] == 'r' ? "rup" : "up");
            steps[step_count++] = up;
        }
    }
    free(copy);
    for (int i = 1; i < step_count; i++) {          /* in time order, keeping the order of equal times */
        Step s = steps[i];
        int j = i;
        while (j > 0 && steps[j - 1].at > s.at) { steps[j] = steps[j - 1]; j--; }
        steps[j] = s;
    }
}

static void run_script(void)
{
    if (!script_loaded) load_script();
    uint64_t now = SDL_GetTicks64() - opened_at;
    while (step_next < step_count && steps[step_next].at <= now) {
        Step *s = &steps[step_next++];
        fprintf(stderr, "recomp: script %llu %s %d %d %s\n", (unsigned long long)s->at, s->action, s->a, s->b, s->text);
        if (!strcmp(s->action, "move")) post_mouse(WM_MOUSEMOVE, s->a, s->b);
        else if (!strcmp(s->action, "click") || !strcmp(s->action, "down")) {
            post_mouse(WM_MOUSEMOVE, s->a, s->b);
            buttons |= 1;
            post_mouse(WM_LBUTTONDOWN, s->a, s->b);
        } else if (!strcmp(s->action, "up")) {
            buttons &= ~1;
            post_mouse(WM_LBUTTONUP, s->a, s->b);
        } else if (!strcmp(s->action, "rclick")) {
            post_mouse(WM_MOUSEMOVE, s->a, s->b);
            buttons |= 2;
            post_mouse(WM_RBUTTONDOWN, s->a, s->b);
        } else if (!strcmp(s->action, "rup")) {
            buttons &= ~2;
            post_mouse(WM_RBUTTONUP, s->a, s->b);
        } else if (!strcmp(s->action, "key")) {
            post(WM_KEYDOWN, (uint32_t)s->a, 1);
            post(WM_KEYUP, (uint32_t)s->a, 0xC0000001u);
        } else if (!strcmp(s->action, "char")) post(WM_CHAR, (uint32_t)s->a, 1);
        else if (!strcmp(s->action, "dump")) { present_now(); win_dump_screen(s->text); }
        else if (!strcmp(s->action, "quit")) {
            if (rt_on_fatal) rt_on_fatal();
            SDL_Quit();
            exit(0);
        }
    }
}

void win_poll_events(void)
{
    uint64_t now = SDL_GetTicks64();
    if (win_screen_dirty && now - last_present >= 16) present_now();
    if (now - last_poll < 2) return;              /* the game pumps very often; SDL needs less */
    last_poll = now;
    SDL_Event e;
    while (SDL_PollEvent(&e)) {
        switch (e.type) {
        case SDL_QUIT:
            if (rt_on_fatal) rt_on_fatal();
            SDL_Quit();
            exit(0);
        case SDL_MOUSEMOTION:
            post_mouse(WM_MOUSEMOVE, e.motion.x, e.motion.y);
            break;
        case SDL_MOUSEBUTTONDOWN:
        case SDL_MOUSEBUTTONUP: {
            int down = e.type == SDL_MOUSEBUTTONDOWN;
            uint32_t bit = e.button.button == SDL_BUTTON_LEFT ? 1u : e.button.button == SDL_BUTTON_RIGHT ? 2u : 0u;
            if (!bit) break;
            if (down) buttons |= (int)bit; else buttons &= ~(int)bit;
            uint32_t msg = bit == 1 ? (down ? WM_LBUTTONDOWN : WM_LBUTTONUP) : (down ? WM_RBUTTONDOWN : WM_RBUTTONUP);
            post_mouse(msg, e.button.x, e.button.y);
            break;
        }
        case SDL_MOUSEWHEEL:
            if (e.wheel.y) post(WM_MOUSEWHEEL, ((uint32_t)(e.wheel.y * 120) & 0xffff) << 16 | (uint32_t)buttons, mouse_lparam());
            break;
        case SDL_KEYDOWN:
        case SDL_KEYUP: {
            uint32_t vk = virtual_key(e.key.keysym.sym);
            if (!vk) break;
            int down = e.type == SDL_KEYDOWN;
            post(down ? WM_KEYDOWN : WM_KEYUP, vk, down ? 1u : 0xC0000001u);
            /* TranslateMessage gives these keys a character; SDL gives none. */
            if (down && (vk == 0x08 || vk == 0x09 || vk == 0x0D || vk == 0x1B)) post(WM_CHAR, vk, 1);
            break;
        }
        case SDL_TEXTINPUT: {
            const unsigned char *t = (const unsigned char *)e.text.text;
            /* UTF-8 to code page 1252 for the Latin-1 range. */
            for (; *t; t++) {
                uint32_t c = *t;
                if (c >= 0xC0 && c < 0xE0 && (t[1] & 0xC0) == 0x80) { c = ((c & 0x1F) << 6) | (t[1] & 0x3F); t++; }
                else if (c >= 0x80) continue;
                if (c < 0x100) post(WM_CHAR, c, 1);
            }
            break;
        }
        case SDL_WINDOWEVENT:
            if (e.window.event == SDL_WINDOWEVENT_EXPOSED) win_screen_dirty = 1;
            break;
        }
    }
    if (window) run_script();
}

/* PeekMessageA(msg, hwnd, first, last, remove) */
RT_IMPORT(PeekMessageA)
{
    uint32_t m = rt_arg(0), remove = rt_arg(4);
    win_poll_events();
    if (q_head == q_tail) { rt_ret_stdcall(0, 5); return; }
    Msg *q = &queue[q_head % QUEUE];
    wr32(g_mem, m, win_hwnd);
    wr32(g_mem, m + 4, q->msg);
    wr32(g_mem, m + 8, q->wparam);
    wr32(g_mem, m + 12, q->lparam);
    wr32(g_mem, m + 16, q->time);
    wr32(g_mem, m + 20, (uint32_t)mouse_x);
    wr32(g_mem, m + 24, (uint32_t)mouse_y);
    if (remove & 1) q_head++;
    rt_ret_stdcall(1, 5);
}

/* GetMessageA(msg, hwnd, first, last): waits for a message. */
RT_IMPORT(GetMessageA)
{
    uint32_t m = rt_arg(0);
    while (q_head == q_tail) {
        win_poll_events();
        if (q_head == q_tail) SDL_Delay(1);
    }
    Msg *q = &queue[q_head++ % QUEUE];
    wr32(g_mem, m, win_hwnd);
    wr32(g_mem, m + 4, q->msg);
    wr32(g_mem, m + 8, q->wparam);
    wr32(g_mem, m + 12, q->lparam);
    wr32(g_mem, m + 16, q->time);
    wr32(g_mem, m + 20, (uint32_t)mouse_x);
    wr32(g_mem, m + 24, (uint32_t)mouse_y);
    rt_ret_stdcall(q->msg != WM_QUIT, 4);
}

RT_IMPORT(TranslateMessage) { rt_ret_stdcall(1, 1); }

RT_IMPORT(DispatchMessageA)
{
    uint32_t m = rt_arg(0);
    uint32_t args[4] = {rd32(g_mem, m), rd32(g_mem, m + 4), rd32(g_mem, m + 8), rd32(g_mem, m + 12)};
    if (!win_wndproc) rt_fatal("DispatchMessageA before RegisterClassA");
    uint32_t r = rt_callback(win_wndproc, 4, args);
    if (rt_unwinding) return;
    rt_ret_stdcall(r, 1);
}

RT_IMPORT(DefWindowProcA) { rt_ret_stdcall(0, 4); }

/* WNDCLASSA: the window procedure is at +4. */
RT_IMPORT(RegisterClassA)
{
    win_wndproc = rd32(g_mem, rt_arg(0) + 4);
    WIN_TRACE("RegisterClassA: window procedure %#x\n", win_wndproc);
    rt_ret_stdcall(0xC001u, 1);
}

/* CreateWindowExA(ex style, class, title, style, x, y, w, h, parent, menu, instance, param) */
RT_IMPORT(CreateWindowExA)
{
    win_video_init();
    if (rt_arg(2)) SDL_SetWindowTitle(window, win_str(rt_arg(2)));
    rt_ret_stdcall(win_hwnd, 12);
}

RT_IMPORT(ShowWindow)
{
    if (window) { if (rt_arg(1)) SDL_ShowWindow(window); else SDL_HideWindow(window); }
    rt_ret_stdcall(1, 2);
}
RT_IMPORT(UpdateWindow) { rt_ret_stdcall(1, 1); }
RT_IMPORT(MoveWindow) { rt_ret_stdcall(1, 6); }
RT_IMPORT(SetWindowLongA) { rt_ret_stdcall(0, 3); }
RT_IMPORT(DestroyWindow) { rt_ret_stdcall(1, 1); }
RT_IMPORT(CloseWindow) { rt_ret_stdcall(1, 1); }
RT_IMPORT(ChangeDisplaySettingsA) { rt_ret_stdcall(0, 2); }
RT_IMPORT(GetDesktopWindow) { rt_ret_stdcall(DESKTOP_HWND, 0); }

/* The screen and the window are both 800x600 with no frame. */
RT_IMPORT(GetSystemMetrics)
{
    uint32_t i = rt_arg(0);
    rt_ret_stdcall(i == 0 ? 800u : i == 1 ? 600u : 0u, 1);
}

RT_IMPORT(GetWindowRect)
{
    uint32_t r = rt_arg(1);
    wr32(g_mem, r, 0);
    wr32(g_mem, r + 4, 0);
    wr32(g_mem, r + 8, 800);
    wr32(g_mem, r + 12, 600);
    rt_ret_stdcall(1, 2);
}

/* BeginPaint(hwnd, PAINTSTRUCT): a fake device context. */
RT_IMPORT(BeginPaint)
{
    uint32_t ps = rt_arg(1);
    memset(g_mem + ps, 0, 64);
    wr32(g_mem, ps, 0x00020001u);
    wr32(g_mem, ps + 16, 800);
    wr32(g_mem, ps + 20, 600);
    rt_ret_stdcall(0x00020001u, 2);
}
RT_IMPORT(EndPaint) { rt_ret_stdcall(1, 2); }

/* Cursors: a handle per resource; the system arrow and wait cursors stand in for the images. */
#define CURSOR_BASE 0x00030000u
static uint32_t current_cursor;
static int cursor_count;                          /* the ShowCursor counter; shown while >= 0 */

RT_IMPORT(LoadCursorA)
{
    uint32_t id = rt_arg(1);
    rt_ret_stdcall(CURSOR_BASE + (id & 0xffff), 2);
}
RT_IMPORT(LoadIconA) { rt_ret_stdcall(0x00040000u + (rt_arg(1) & 0xffff), 2); }
RT_IMPORT(DestroyCursor) { rt_ret_stdcall(1, 1); }
RT_IMPORT(GetCursor) { rt_ret_stdcall(current_cursor, 0); }

RT_IMPORT(SetCursor)
{
    static SDL_Cursor *arrow, *wait;
    uint32_t h = rt_arg(0), old = current_cursor;
    current_cursor = h;
    if (window && h) {
        int is_wait = h == CURSOR_BASE + 0x7f02;  /* IDC_WAIT */
        SDL_Cursor **c = is_wait ? &wait : &arrow;
        if (!*c) *c = SDL_CreateSystemCursor(is_wait ? SDL_SYSTEM_CURSOR_WAIT : SDL_SYSTEM_CURSOR_ARROW);
        if (*c) SDL_SetCursor(*c);
    }
    rt_ret_stdcall(old, 1);
}

RT_IMPORT(ShowCursor)
{
    cursor_count += rt_arg(0) ? 1 : -1;
    SDL_ShowCursor(cursor_count >= 0 ? SDL_ENABLE : SDL_DISABLE);
    rt_ret_stdcall((uint32_t)cursor_count, 1);
}

/* MessageBoxA(hwnd, text, caption, type): print it, show it, answer OK. */
RT_IMPORT(MessageBoxA)
{
    const char *text = rt_arg(1) ? win_str(rt_arg(1)) : "";
    const char *caption = rt_arg(2) ? win_str(rt_arg(2)) : "";
    fprintf(stderr, "recomp: message box \"%s\": %s\n", caption, text);
    if (!getenv("RECOMP_NO_BOX")) SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_INFORMATION, caption, text, window);
    rt_ret_stdcall(1, 4);
}

/* English (UK). */
RT_IMPORT(GetKeyboardLayout) { rt_ret_stdcall(0x08090809u, 1); }
