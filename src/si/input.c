#include "cm_types.h"
#include "display.h"
#include "scrman_input.h"
#include <SDL2/SDL.h>

static bool g_keys[SDL_NUM_SCANCODES];
static bool g_mouse_buttons[6];

void input_init(void)
{
    memset(g_keys, 0, sizeof(g_keys));
    memset(g_mouse_buttons, 0, sizeof(g_mouse_buttons));
}

/* Mouse x and y in the 800x600 screen. SDL scales them to the logical size. */
static void set_mouse(s32 x, s32 y)
{
    g_mouse_x = x < 0 ? 0 : x > CM_INTERNAL_WIDTH - 1 ? CM_INTERNAL_WIDTH - 1 : x;
    g_mouse_y = y < 0 ? 0 : y > CM_INTERNAL_HEIGHT - 1 ? CM_INTERNAL_HEIGHT - 1 : y;
}

/* The key modes of the window procedure for VK_PRIOR to VK_DOWN, -1 = none. */
static s32 key_mode(SDL_Keycode key)
{
    switch (key) {
    case SDLK_PAGEUP: return 3;
    case SDLK_PAGEDOWN: return 2;
    case SDLK_END: return 4;
    case SDLK_HOME: return 5;
    case SDLK_UP: return 1;
    case SDLK_DOWN: return 0;
    default: return -1;
    }
}

/*
 * The Windows virtual key code of an SDL key, for the US keyboard layout.
 * 0 for a key that the port does not map: Alt and F10 give WM_SYSKEYDOWN in
 * Windows, which the window procedure does not read.
 */
static s32 virtual_key(SDL_Keycode key)
{
    static const struct {
        SDL_Keycode key;
        s32 vk;
    } keys[] = {
        {SDLK_BACKSPACE, 0x08}, {SDLK_TAB, 0x09}, {SDLK_RETURN, 0x0d},
        {SDLK_KP_ENTER, 0x0d}, {SDLK_LSHIFT, 0x10}, {SDLK_RSHIFT, 0x10},
        {SDLK_LCTRL, 0x11}, {SDLK_RCTRL, 0x11}, {SDLK_CAPSLOCK, 0x14},
        {SDLK_ESCAPE, 0x1b}, {SDLK_SPACE, 0x20}, {SDLK_PAGEUP, 0x21},
        {SDLK_PAGEDOWN, 0x22}, {SDLK_END, 0x23}, {SDLK_HOME, 0x24},
        {SDLK_LEFT, 0x25}, {SDLK_UP, 0x26}, {SDLK_RIGHT, 0x27},
        {SDLK_DOWN, 0x28}, {SDLK_INSERT, 0x2d}, {SDLK_DELETE, 0x2e},
        {SDLK_KP_MULTIPLY, 0x6a}, {SDLK_KP_PLUS, 0x6b},
        {SDLK_KP_MINUS, 0x6d}, {SDLK_KP_PERIOD, 0x6e},
        {SDLK_KP_DIVIDE, 0x6f}, {SDLK_F11, 0x7a}, {SDLK_F12, 0x7b},
        {SDLK_SEMICOLON, 0xba}, {SDLK_EQUALS, 0xbb}, {SDLK_COMMA, 0xbc},
        {SDLK_MINUS, 0xbd}, {SDLK_PERIOD, 0xbe}, {SDLK_SLASH, 0xbf},
        {SDLK_BACKQUOTE, 0xc0}, {SDLK_LEFTBRACKET, 0xdb},
        {SDLK_BACKSLASH, 0xdc}, {SDLK_RIGHTBRACKET, 0xdd},
        {SDLK_QUOTE, 0xde},
    };

    if (key >= SDLK_0 && key <= SDLK_9) return 0x30 + (key - SDLK_0);
    if (key >= SDLK_a && key <= SDLK_z) return 0x41 + (key - SDLK_a);
    if (key >= SDLK_F1 && key <= SDLK_F9) return 0x70 + (key - SDLK_F1);
    if (key >= SDLK_KP_1 && key <= SDLK_KP_9) return 0x61 + (key - SDLK_KP_1);
    if (key == SDLK_KP_0) return 0x60;
    for (size_t i = 0; i < sizeof(keys) / sizeof(keys[0]); i++) {
        if (keys[i].key == key) return keys[i].vk;
    }
    return 0;
}

/*
 * The character of an SDL text event. Windows gives one WM_CHAR for each
 * character, so the last one wins. A character above U+00FF has no byte in
 * the Latin-1 set, and the port drops it.
 */
static void set_char(const char *text)
{
    const u8 *c = (const u8 *)text;

    while (*c) {
        if (*c < 0x80) {
            g_key_char = *c++;
        } else if ((*c & 0xe0) == 0xc0 && (c[1] & 0xc0) == 0x80) {
            u32 code = (u32)(*c & 0x1f) << 6 | (c[1] & 0x3f);

            if (code < 0x100) g_key_char = (u8)code;
            c += 2;
        } else {
            c++;
            while ((*c & 0xc0) == 0x80) c++;
        }
    }
}

/*
 * Sets the mouse globals of the original as its window procedure does for
 * WM_MOUSEMOVE, WM_LBUTTONDOWN, WM_LBUTTONUP, WM_RBUTTONDOWN, WM_RBUTTONUP,
 * WM_MOUSEWHEEL, WM_KEYDOWN and WM_CHAR. While a click of the keys or the
 * wheel lasts (g_click_frames), the mouse and the wheel do nothing.
 *
 * A key gives its Windows key code (g_key_code). SDL gives no text for
 * Backspace, Tab, Enter and Escape, where Windows gives a WM_CHAR, so the
 * port sets their character from the key.
 */
void input_poll(SDL_Event *e)
{
    bool mouse_free = g_click_frames == 0;

    switch (e->type) {
    case SDL_MOUSEMOTION:
        if (mouse_free) set_mouse(e->motion.x, e->motion.y);
        break;
    case SDL_TEXTINPUT:
        set_char(e->text.text);
        break;
    case SDL_KEYDOWN: {
        s32 vk = virtual_key(e->key.keysym.sym);

        if (vk != 0) g_key_code = vk;
        if (vk == 0x08 || vk == 0x09 || vk == 0x0d || vk == 0x1b) {
            g_key_char = (u8)vk;
        }
        if (key_mode(e->key.keysym.sym) != -1) {
            scrman_scroll_key(key_mode(e->key.keysym.sym), 1);
        }
        break;
    }
    case SDL_MOUSEWHEEL:
        /* One notch is one frame. Up is mode 1. SDL gives 0 for part of a notch. */
        if (mouse_free && e->wheel.y != 0) {
            scrman_scroll_key(e->wheel.y > 0 ? 1 : 0,
                              e->wheel.y > 0 ? e->wheel.y : -e->wheel.y);
        }
        break;
    default:
        break;
    }
    if (mouse_free &&
        (e->type == SDL_MOUSEBUTTONDOWN || e->type == SDL_MOUSEBUTTONUP)) {
        bool down = e->type == SDL_MOUSEBUTTONDOWN;

        set_mouse(e->button.x, e->button.y);
        if (e->button.button == SDL_BUTTON_LEFT) {
            g_left_held = down;
            if (down) g_left_clicked = 1;
        } else if (e->button.button == SDL_BUTTON_RIGHT) {
            g_right_held = down;
            if (down) g_right_clicked = 1;
        }
    }
    switch (e->type) {
    case SDL_KEYDOWN:
        if (e->key.keysym.scancode < SDL_NUM_SCANCODES)
            g_keys[e->key.keysym.scancode] = true;
        break;
    case SDL_KEYUP:
        if (e->key.keysym.scancode < SDL_NUM_SCANCODES)
            g_keys[e->key.keysym.scancode] = false;
        break;
    case SDL_MOUSEBUTTONDOWN:
        if (e->button.button < 6)
            g_mouse_buttons[e->button.button] = true;
        break;
    case SDL_MOUSEBUTTONUP:
        if (e->button.button < 6)
            g_mouse_buttons[e->button.button] = false;
        break;
    default:
        break;
    }
}

bool input_key_down(s32 scancode)
{
    if (scancode < 0 || scancode >= SDL_NUM_SCANCODES) return false;
    return g_keys[scancode];
}

bool input_key_pressed(s32 scancode)
{
    if (scancode < 0 || scancode >= SDL_NUM_SCANCODES) return false;
    bool state = g_keys[scancode];
    g_keys[scancode] = false;
    return state;
}

bool input_mouse_down(s32 button)
{
    if (button < 0 || button > 5) return false;
    return g_mouse_buttons[button];
}

void input_get_mouse(s32 *x, s32 *y)
{
    if (x) *x = g_display.mouse_x;
    if (y) *y = g_display.mouse_y;
}
