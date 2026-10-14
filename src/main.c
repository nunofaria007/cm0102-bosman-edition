#include "cm_types.h"
#include "demo.h"
#include "display.h"
#include "gui_font.h"
#include "gui_utils.h"
#include "scrman.h"
#include "screen_stack.h"
#include "setup.h"
#include "utils.h"
#include "guio.h"
#include <time.h>
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
static scrman_t g_screen_manager;

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

/*
 * The message pump of the screen stack: SDL events set the mouse globals,
 * and the tick of the original is the SDL tick. Returns false on quit.
 */
static bool pump(void)
{
    SDL_Event e;

    while (SDL_PollEvent(&e)) {
        handle_event(&e);
        input_poll(&e);
        /*
         * A quick tap gives the press and the release in one pass. The poll
         * must see the button held once, so the release waits for the next pass.
         */
        if (e.type == SDL_MOUSEBUTTONDOWN) break;
    }
    g_tick = (s32)SDL_GetTicks();
    SDL_Delay(5);
    return g_running;
}

/* Runs the screens of the original, starting with "Setup Game". */
static void run_game(const char *pictures_dir)
{
    g_pictures_dir = pictures_dir;
    scrman_init(&g_screen_manager, 0);
    scrman_reset(&g_screen_manager, true);
    screen_stack_init(&g_screen_stack, &g_screen_manager);
    setup_push_start_screen();
    g_screen_pump = pump;
    SDL_StartTextInput();
    screen_stack_run(&g_screen_stack);
}

/* Usage: cm0102 [--data <folder>] [--pictures <folder>] [--demo] [picture.rgn] */
int main(int argc, char *argv[])
{
    const char *data_dir = "./Data";
    const char *pictures_dir = NULL;
    const char *picture = NULL;
    static char default_pictures[1024];

    for (int i = 1; i < argc; i++) {
        if (!strcmp(argv[i], "--demo")) {
            g_demo = true;
        } else if (!strcmp(argv[i], "--data") && i + 1 < argc) {
            data_dir = argv[++i];
        } else if (!strcmp(argv[i], "--pictures") && i + 1 < argc) {
            pictures_dir = argv[++i];
        } else if (argv[i][0] == '-') {
            fprintf(stderr, "Usage: cm0102 [--data <folder>] "
                            "[--pictures <folder>] [--demo] [picture.rgn]\n");
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
        g_data_dir = data_dir;
        demo_draw(picture);
    } else if (!picture) {
        /* The Pictures folder sits next to the data folder of the original. */
        if (!pictures_dir) {
            snprintf(default_pictures, sizeof(default_pictures), "%s/../Pictures",
                     data_dir);
            pictures_dir = default_pictures;
        }
        if (!gui_fonts_load(data_dir)) {
            fprintf(stderr, "No fonts in %s. Use --data <folder>.\n", data_dir);
            display_shutdown();
            return 1;
        }
        g_data_dir = data_dir;
        srand((unsigned)time(NULL));
        run_game(pictures_dir);
        g_running = false;
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
