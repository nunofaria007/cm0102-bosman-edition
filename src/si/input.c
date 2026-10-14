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
 * Sets the mouse globals of the original as its window procedure does for
 * WM_MOUSEMOVE, WM_LBUTTONDOWN, WM_LBUTTONUP, WM_RBUTTONDOWN, WM_RBUTTONUP,
 * WM_MOUSEWHEEL, WM_KEYDOWN and WM_CHAR. While a click of the keys or the
 * wheel lasts (g_click_frames), the mouse and the wheel do nothing.
 */
void input_poll(SDL_Event *e)
{
    bool mouse_free = g_click_frames == 0;

    switch (e->type) {
    case SDL_MOUSEMOTION:
        if (mouse_free) set_mouse(e->motion.x, e->motion.y);
        break;
    case SDL_TEXTINPUT:
        g_key_char = (u8)e->text.text[0];
        break;
    case SDL_KEYDOWN:
        if (key_mode(e->key.keysym.sym) != -1) {
            scrman_scroll_key(key_mode(e->key.keysym.sym), 1);
        }
        break;
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
