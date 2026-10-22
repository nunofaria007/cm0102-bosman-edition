#include "display.h"
#include <stdlib.h>
#include <string.h>

display_t g_display = {0};

bool display_init(const char *title, bool fullscreen)
{
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        SDL_Log("SDL_Init failed: %s", SDL_GetError());
        return false;
    }

    u32 flags = SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI;
    s32 init_w = CM_INTERNAL_WIDTH * 2;
    s32 init_h = CM_INTERNAL_HEIGHT * 2;

    if (fullscreen) {
        flags |= SDL_WINDOW_FULLSCREEN_DESKTOP;
    }

    g_display.window = SDL_CreateWindow(
        title,
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        init_w, init_h, flags
    );
    if (!g_display.window) {
        SDL_Log("SDL_CreateWindow failed: %s", SDL_GetError());
        return false;
    }

#ifdef __EMSCRIPTEN__
    u32 renderer_flags = SDL_RENDERER_SOFTWARE;
#else
    u32 renderer_flags = SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC;
#endif
    g_display.renderer = SDL_CreateRenderer(g_display.window, -1, renderer_flags);
    if (!g_display.renderer) {
        SDL_Log("SDL_CreateRenderer failed: %s", SDL_GetError());
        return false;
    }

    SDL_RenderSetLogicalSize(g_display.renderer,
                             CM_INTERNAL_WIDTH, CM_INTERNAL_HEIGHT);
    SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "nearest");

    g_display.screen_tex = SDL_CreateTexture(
        g_display.renderer,
        SDL_PIXELFORMAT_RGB565,
        SDL_TEXTUREACCESS_STREAMING,
        CM_INTERNAL_WIDTH, CM_INTERNAL_HEIGHT
    );
    if (!g_display.screen_tex) {
        SDL_Log("SDL_CreateTexture failed: %s", SDL_GetError());
        return false;
    }

    g_display.framebuffer = calloc(CM_INTERNAL_WIDTH * CM_INTERNAL_HEIGHT,
                                   sizeof(u16));
    if (!g_display.framebuffer) {
        return false;
    }

    g_display.pitch       = CM_INTERNAL_WIDTH * sizeof(u16);
    g_display.fullscreen  = fullscreen;
    g_display.window_w    = init_w;
    g_display.window_h    = init_h;
    g_display.dirty       = true;

    return true;
}

void display_shutdown(void)
{
    free(g_display.framebuffer);
    g_display.framebuffer = NULL;

    if (g_display.screen_tex)  SDL_DestroyTexture(g_display.screen_tex);
    if (g_display.renderer)    SDL_DestroyRenderer(g_display.renderer);
    if (g_display.window)      SDL_DestroyWindow(g_display.window);

    g_display.screen_tex = NULL;
    g_display.renderer   = NULL;
    g_display.window     = NULL;

    SDL_Quit();
}

void display_lock(void)
{
    /* No-op: we write directly to our own framebuffer. */
}

void display_unlock(void)
{
    /* No-op: paired with display_lock. */
}

void (*display_on_flip)(s32 x1, s32 y1, s32 x2, s32 y2);

void display_flip(s32 x1, s32 y1, s32 x2, s32 y2)
{
    if (display_on_flip) display_on_flip(x1, y1, x2, y2);
    if (!g_display.renderer) return;

    SDL_UpdateTexture(g_display.screen_tex, NULL,
                      g_display.framebuffer, g_display.pitch);
    SDL_RenderClear(g_display.renderer);
    SDL_RenderCopy(g_display.renderer, g_display.screen_tex, NULL, NULL);
    SDL_RenderPresent(g_display.renderer);

    g_display.dirty = false;
}

void display_clear(u16 colour)
{
    for (s32 i = 0; i < CM_INTERNAL_WIDTH * CM_INTERNAL_HEIGHT; i++) {
        g_display.framebuffer[i] = colour;
    }
    g_display.dirty = true;
}

void display_fill_rect(s32 x, s32 y, s32 w, s32 h, u16 colour)
{
    for (s32 row = y; row < y + h && row < CM_INTERNAL_HEIGHT; row++) {
        if (row < 0) continue;
        for (s32 col = x; col < x + w && col < CM_INTERNAL_WIDTH; col++) {
            if (col < 0) continue;
            g_display.framebuffer[row * CM_INTERNAL_WIDTH + col] = colour;
        }
    }
    g_display.dirty = true;
}

void display_blit_region(s32 dx, s32 dy, s32 sx, s32 sy,
                         s32 w, s32 h, const cm_image_t *img)
{
    if (!img || !img->pixels) return;

    for (s32 row = 0; row < h; row++) {
        s32 src_y = sy + row;
        s32 dst_y = dy + row;
        if (src_y < 0 || src_y >= img->height) continue;
        if (dst_y < 0 || dst_y >= CM_INTERNAL_HEIGHT) continue;

        for (s32 col = 0; col < w; col++) {
            s32 src_x = sx + col;
            s32 dst_x = dx + col;
            if (src_x < 0 || src_x >= img->width) continue;
            if (dst_x < 0 || dst_x >= CM_INTERNAL_WIDTH) continue;

            u16 pixel = img->pixels[src_y * img->width + src_x];
            if (pixel != 0) {
                g_display.framebuffer[dst_y * CM_INTERNAL_WIDTH + dst_x] = pixel;
            }
        }
    }
    g_display.dirty = true;
}

void display_toggle_fullscreen(void)
{
    g_display.fullscreen = !g_display.fullscreen;
    SDL_SetWindowFullscreen(
        g_display.window,
        g_display.fullscreen ? SDL_WINDOW_FULLSCREEN_DESKTOP : 0
    );
}

void display_set_window_size(s32 w, s32 h)
{
    if (!g_display.fullscreen) {
        SDL_SetWindowSize(g_display.window, w, h);
        SDL_SetWindowPosition(g_display.window,
                              SDL_WINDOWPOS_CENTERED,
                              SDL_WINDOWPOS_CENTERED);
        g_display.window_w = w;
        g_display.window_h = h;
    }
}
