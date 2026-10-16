/* The size of a new window on a small screen (0.4.0 slice 10).
 *
 * A new window opens at the size of the game's screen: 1920x1080 in HD mode. On a 1080p screen the
 * menu bar or the task bar and the title bar take part of the height, so the window would go past the
 * edge of the screen. winfit_size makes the window smaller, with the same shape, until it fits in
 * max_w x max_h. The picture is scaled to the window as before (user32.c). */
#ifndef WINFIT_H
#define WINFIT_H

static inline void winfit_size(int w, int h, int max_w, int max_h, int *out_w, int *out_h)
{
    *out_w = w;
    *out_h = h;
    if (max_w <= 0 || max_h <= 0 || (w <= max_w && h <= max_h)) return;
    if ((long long)w * max_h > (long long)h * max_w) {      /* the width limits */
        *out_w = max_w;
        *out_h = (int)((long long)h * max_w / w);
    } else {                                                /* the height limits */
        *out_h = max_h;
        *out_w = (int)((long long)w * max_h / h);
    }
}

#endif
