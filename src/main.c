#include "cm_types.h"
#include "demo.h"
#include "display.h"
#include "gui_font.h"
#include "gui_utils.h"
#include <SDL2/SDL.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

extern void sound_init(void);
extern void sound_shutdown(void);
extern void input_init(void);
extern void input_poll(SDL_Event *e);
extern cm_image_t *rgn_load(const char *path);
extern void rgn_free(cm_image_t *img);

static bool g_running = true;
static bool g_demo = false;

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
        if (g_demo && e->key.keysym.sym == SDLK_UP) demo_scroll(-1);
        if (g_demo && e->key.keysym.sym == SDLK_DOWN) demo_scroll(1);
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

/* Usage: cm0102 [--data <folder>] [--demo] [picture.rgn] */
int main(int argc, char *argv[])
{
    const char *data_dir = "./Data";
    const char *picture = NULL;

    for (int i = 1; i < argc; i++) {
        if (!strcmp(argv[i], "--demo")) {
            g_demo = true;
        } else if (!strcmp(argv[i], "--data") && i + 1 < argc) {
            data_dir = argv[++i];
        } else if (argv[i][0] == '-') {
            fprintf(stderr, "Usage: cm0102 [--data <folder>] [--demo] "
                            "[picture.rgn]\n");
            return 2;
        } else {
            picture = argv[i];
        }
    }

    if (!display_init("Championship Manager 01/02", false)) {
        fprintf(stderr, "Display init failed\n");
        return 1;
    }

    sound_init();
    input_init();

    display_clear(0x0000);

    if (g_demo) {
        if (!gui_fonts_load(data_dir)) {
            fprintf(stderr, "No fonts in %s. Use --data <folder>.\n", data_dir);
            display_shutdown();
            return 1;
        }
        demo_draw(data_dir, picture);
    } else if (picture) {
        cm_image_t *bg = rgn_load(picture);
        if (bg) {
            printf("Loaded RGN: %dx%d, data_size=%d, g_mask=0x%04x\n",
                   bg->width, bg->height, bg->data_size, bg->g_mask);
            gui_blit(0, 0, bg);
        } else {
            fprintf(stderr, "Failed to load: %s\n", picture);
        }
    }

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
