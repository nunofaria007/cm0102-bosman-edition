#ifndef DISPLAY_H
#define DISPLAY_H

#include "cm_types.h"
#include <SDL2/SDL.h>

/*
 * Display subsystem.
 *
 * Replaces the original DirectDraw rendering path (display.cpp / Guio.cpp).
 * The game draws to an internal 800x600 16-bit surface. SDL2 scales that
 * surface to whatever window or fullscreen resolution the user picks.
 */

typedef struct {
    SDL_Window   *window;
    SDL_Renderer *renderer;
    SDL_Texture  *screen_tex;    /* 800x600 RGB565 texture */
    u16          *framebuffer;   /* 800x600 back buffer pixels */
    s32           pitch;         /* bytes per row in framebuffer */

    bool fullscreen;
    s32  window_w;
    s32  window_h;
    s32  scale;                  /* integer scale factor */

    s32  mouse_x;               /* scaled to 0..799 */
    s32  mouse_y;               /* scaled to 0..599 */

    bool cursor_visible;
    bool dirty;
} display_t;

extern display_t g_display;

/* Lifecycle */
bool display_init(const char *title, bool fullscreen);
void display_shutdown(void);

/* Frame operations */
void display_lock(void);
void display_unlock(void);
void display_flip(s32 x1, s32 y1, s32 x2, s32 y2);

/* Drawing primitives (write to back buffer) */
void display_clear(u16 colour);
void display_fill_rect(s32 x, s32 y, s32 w, s32 h, u16 colour);
void display_blit_image(s32 dest_x, s32 dest_y, const cm_image_t *img);
void display_blit_region(s32 dx, s32 dy, s32 sx, s32 sy,
                         s32 w, s32 h, const cm_image_t *img);

/* Pixel access */
static inline void display_put_pixel(s32 x, s32 y, u16 colour) {
    if (x >= 0 && x < CM_INTERNAL_WIDTH && y >= 0 && y < CM_INTERNAL_HEIGHT) {
        g_display.framebuffer[y * CM_INTERNAL_WIDTH + x] = colour;
    }
}

static inline u16 display_get_pixel(s32 x, s32 y) {
    if (x >= 0 && x < CM_INTERNAL_WIDTH && y >= 0 && y < CM_INTERNAL_HEIGHT) {
        return g_display.framebuffer[y * CM_INTERNAL_WIDTH + x];
    }
    return 0;
}

/* Windowed / fullscreen toggle */
void display_toggle_fullscreen(void);
void display_set_window_size(s32 w, s32 h);

#endif
