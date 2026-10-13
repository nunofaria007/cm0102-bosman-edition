#include "cm_types.h"
#include "display.h"
#include <SDL2/SDL.h>

static bool g_keys[SDL_NUM_SCANCODES];
static bool g_mouse_buttons[6];

void input_init(void)
{
    memset(g_keys, 0, sizeof(g_keys));
    memset(g_mouse_buttons, 0, sizeof(g_mouse_buttons));
}

void input_poll(SDL_Event *e)
{
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
