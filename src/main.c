#include "cm_types.h"
#include "display.h"
#include <SDL2/SDL.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

extern void sound_init(void);
extern void sound_shutdown(void);
extern void input_init(void);
extern void input_poll(SDL_Event *e);

static bool g_running = true;

static void handle_event(SDL_Event *e)
{
    switch (e->type) {
    case SDL_QUIT:
        g_running = false;
        break;

    case SDL_KEYDOWN:
        if (e->key.keysym.sym == SDLK_RETURN &&
            (e->key.keysym.mod & KMOD_ALT)) {
            display_toggle_fullscreen();
        }
        if (e->key.keysym.sym == SDLK_ESCAPE) {
            g_running = false;
        }
        break;

    case SDL_WINDOWEVENT:
        if (e->window.event == SDL_WINDOWEVENT_RESIZED) {
            g_display.window_w = e->window.data1;
            g_display.window_h = e->window.data2;
        }
        break;

    case SDL_MOUSEMOTION:
        g_display.mouse_x = e->motion.x;
        g_display.mouse_y = e->motion.y;
        break;

    default:
        break;
    }
}

int main(int argc, char *argv[])
{
    (void)argc;
    (void)argv;

    if (!display_init("Championship Manager 01/02", false)) {
        fprintf(stderr, "Display init failed\n");
        return 1;
    }

    sound_init();
    input_init();

    display_clear(0x0000);

    while (g_running) {
        SDL_Event e;
        while (SDL_PollEvent(&e)) {
            handle_event(&e);
            input_poll(&e);
        }

        if (g_display.dirty) {
            display_flip(0, 0, CM_INTERNAL_WIDTH - 1, CM_INTERNAL_HEIGHT - 1);
        } else {
            SDL_Delay(1);
        }
    }

    sound_shutdown();
    display_shutdown();

    return 0;
}
