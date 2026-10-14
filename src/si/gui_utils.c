#include "gui_utils.h"
#include "display.h"
#include <stdlib.h>
#include <string.h>

extern void rgn_free(cm_image_t *img);

static inline u16 *pixel_at(s32 x, s32 y)
{
    return &g_display.framebuffer[y * CM_INTERNAL_WIDTH + x];
}

bool gui_clip(s32 x1, s32 y1, s32 x2, s32 y2, gui_rect_t *out)
{
    bool inside = true;

    if (!out) return false;

    out->x1 = x1 < x2 ? x1 : x2;
    out->y1 = y1 < y2 ? y1 : y2;
    out->x2 = x1 > x2 ? x1 : x2;
    out->y2 = y1 > y2 ? y1 : y2;

    if (out->x1 < 0) {
        out->x1 = 0;
        inside = false;
    }
    if (out->y1 < 0) {
        out->y1 = 0;
        inside = false;
    }
    if (out->x2 > CM_INTERNAL_WIDTH - 1) {
        out->x2 = CM_INTERNAL_WIDTH - 1;
        inside = false;
    }
    if (out->y2 > CM_INTERNAL_HEIGHT - 1) {
        out->y2 = CM_INTERNAL_HEIGHT - 1;
        inside = false;
    }

    return inside;
}

static u32 scale_channel(u16 colour, u32 mask, u32 percent)
{
    u32 value = (((u32)colour & mask) << 8) / (mask + 1) & 0xff;

    value = value * percent / 100;
    return value > 0xff ? 0xff : value;
}

u16 gui_scale_colour(u16 colour, u8 percent)
{
    u32 r = scale_channel(colour, 0xf800, percent);
    u32 g = scale_channel(colour, 0x07e0, percent);
    u32 b = scale_channel(colour, 0x001f, percent);

    return (u16)((((r & 0xf8) << 5 | (g & 0xfc)) << 3) | b >> 3);
}

/* Direction matters: dash phase and rounding start at the lower major end. */
static void diagonal_line(s32 x1, s32 y1, s32 x2, s32 y2, bool solid,
                          u16 colour)
{
    s32 dx = abs(x2 - x1);
    s32 dy = abs(y2 - y1);
    bool steep = dx < dy;

    if (steep ? y1 > y2 : x1 > x2) {
        s32 t = x1;
        x1 = x2;
        x2 = t;
        t = y1;
        y1 = y2;
        y2 = t;
    }

    s32 major = steep ? dy : dx;
    s32 minor = steep ? dx : dy;
    s32 step = steep ? (x2 <= x1 ? -1 : 1) : (y2 <= y1 ? -1 : 1);
    s32 error = 2 * minor - major;
    s32 x = x1, y = y1;

    *pixel_at(x, y) = colour;
    for (s32 count = 1; count <= major; count++) {
        if (error >= 0) {
            if (steep) {
                x += step;
            } else {
                y += step;
            }
            error += 2 * (minor - major);
        } else {
            error += 2 * minor;
        }
        if (steep) {
            y++;
        } else {
            x++;
        }
        if (solid || count % 6 < 4) *pixel_at(x, y) = colour;
    }
}

void gui_line(s32 x1, s32 y1, s32 x2, s32 y2, u32 style, u16 colour)
{
    gui_rect_t c;
    bool solid = (style & GUI_LINE_SOLID) != 0;

    if (!gui_clip(x1, y1, x2, y2, &c)) return;

    if (c.y1 == c.y2) {
        for (s32 x = c.x1; x <= c.x2; x++) {
            if (solid || (x - c.x1) % 6 < 4) *pixel_at(x, c.y1) = colour;
        }
    } else if (c.x1 == c.x2) {
        for (s32 y = c.y1; y <= c.y2; y++) {
            if (solid || (y - c.y1) % 6 < 4) *pixel_at(c.x1, y) = colour;
        }
    } else {
        diagonal_line(x1, y1, x2, y2, solid, colour);
    }
    g_display.dirty = true;
}

void gui_rect(s32 x1, s32 y1, s32 x2, s32 y2, u32 style, u16 colour)
{
    gui_rect_t c;

    if (style & (GUI_LINE_DASHED | GUI_LINE_SOLID)) {
        u32 side = style & GUI_LINE_DASHED ? GUI_LINE_DASHED : GUI_LINE_SOLID;

        gui_line(x1, y1, x2, y1, side, colour);
        gui_line(x2, y1, x2, y2, side, colour);
        gui_line(x2, y2, x1, y2, side, colour);
        gui_line(x1, y2, x1, y1, side, colour);
        return;
    }

    if (!gui_clip(x1, y1, x2, y2, &c)) return;

    for (s32 y = c.y1; y <= c.y2; y++) {
        for (s32 x = c.x1; x <= c.x2; x++) {
            *pixel_at(x, y) = colour;
        }
    }
    g_display.dirty = true;
}

void gui_darken(s32 x1, s32 y1, s32 x2, s32 y2)
{
    gui_rect_t c;

    if (!gui_clip(x1, y1, x2, y2, &c)) return;

    for (s32 y = c.y1; y <= c.y2; y++) {
        for (s32 x = c.x1; x <= c.x2; x++) {
            u16 *p = pixel_at(x, y);
            *p = gui_scale_colour(*p, 60);
        }
    }
    g_display.dirty = true;
}

static u16 or_navy(u16 colour)
{
    return colour ? colour : GUI_COLOUR_NAVY;
}

/* The original divides by the span, so it faults when the span is 0. */
static void panel_gradient(s32 x1, s32 y1, s32 x2, s32 y2, bool across,
                           u16 colour)
{
    s32 span = across ? x2 - x1 : y2 - y1;

    for (s32 i = 0; span > 0 && i <= span; i++) {
        u16 shade = gui_scale_colour(colour, (u8)(100 - 100 * i / span));

        if (across) {
            gui_line(x1 + i, y1, x1 + i, y2, GUI_LINE_SOLID, shade);
        } else {
            gui_line(x1, y1 + i, x2, y1 + i, GUI_LINE_SOLID, shade);
        }
    }
}

static void panel_bevel(s32 x1, s32 y1, s32 x2, s32 y2, u32 flags, u16 colour)
{
    bool thick = x2 - x1 >= 50 && y2 - y1 >= 50 && y2 <= 99 &&
                 (flags & GUI_PANEL_FILL);
    s32 rings = thick ? 4 : 2;

    for (s32 ring = 1; ring <= rings; ring++) {
        s32 inset = rings - ring;
        s32 left = x1 + inset, top = y1 + inset;
        s32 right = x2 - inset, bottom = y2 - inset;
        s32 step = 66 * ring / rings;
        u16 lit = gui_scale_colour(or_navy(colour), (u8)(100 + step));
        u16 shaded = gui_scale_colour(colour, (u8)(100 - step));

        if (flags & GUI_PANEL_SUNKEN) {
            u16 t = lit;
            lit = shaded;
            shaded = t;
        }
        gui_line(left, top, right, top, GUI_LINE_SOLID, lit);
        gui_line(left, top, left, bottom, GUI_LINE_SOLID, lit);
        gui_line(right, bottom, left + 1, bottom, GUI_LINE_SOLID, shaded);
        gui_line(right, bottom, right, top + 1, GUI_LINE_SOLID, shaded);
    }
}

void gui_panel(s32 x1, s32 y1, s32 x2, s32 y2, u32 flags, u16 colour)
{
    gui_rect_t c;
    u32 pen = flags & GUI_PANEL_DASHED ? GUI_LINE_DASHED : GUI_LINE_SOLID;
    bool ellipse = (flags & GUI_PANEL_ELLIPSE) != 0;

    if (flags & GUI_PANEL_SHRINK) {
        x2 -= 2;
        y2 -= 2;
    }
    if (flags & GUI_PANEL_INSET) {
        x1 += 2;
        y1 += 2;
    }
    if (flags & GUI_PANEL_SAMPLE_COLOUR) {
        colour = gui_clip(x1, y1, x1, y1, &c) ? *pixel_at(x1, y1) : 0;
    }

    if (flags & GUI_PANEL_DARKEN) {
        gui_darken(x1, y1, x2, y2);
    } else if (flags & GUI_PANEL_FILL) {
        if (!ellipse) gui_rect(x1, y1, x2, y2, GUI_RECT_FILL, colour);
    } else if (flags & GUI_PANEL_GRADIENT_H) {
        panel_gradient(x1, y1, x2, y2, true, colour);
    } else if (flags & GUI_PANEL_GRADIENT_V) {
        panel_gradient(x1, y1, x2, y2, false, colour);
    }

    if (flags & GUI_PANEL_BORDER) {
        if (ellipse) {
            /* Not ported. */
        } else if (flags & GUI_PANEL_FLAT) {
            gui_rect(x1, y1, x2, y2, pen, colour);
        } else {
            panel_bevel(x1, y1, x2, y2, flags, colour);
        }
    } else if (flags & GUI_PANEL_UNDERLINE) {
        gui_line(x1 + 2, y2, x2 - 2, y2, GUI_LINE_DASHED,
                 gui_scale_colour(or_navy(colour), 90));
    } else if (flags & GUI_PANEL_LINE) {
        if (!(flags & GUI_PANEL_LINE_H)) {
            gui_line(x1, y1, x2, y2, pen, colour);
        } else {
            s32 y = flags & GUI_PANEL_LINE_TOP ? y1 : (y2 - y1) / 2 + y1;
            gui_line(x1, y, x2, y, pen, colour);
        }
    }

    if (flags & GUI_PANEL_GROOVE) {
        s32 y = (y2 - y1) / 2 + y1;

        gui_line(x1 + 2, y, x2 - 2, y, GUI_LINE_SOLID,
                 gui_scale_colour(colour, 66));
        gui_line(x1 + 2, y + 1, x2 - 2, y + 1, GUI_LINE_SOLID,
                 gui_scale_colour(or_navy(colour), 133));
    }
    if (flags & GUI_PANEL_OUTER_OUTLINE) {
        gui_rect(x1 - 1, y1 - 1, x2 + 1, y2 + 1, GUI_LINE_SOLID,
                 GUI_COLOUR_YELLOW);
    }
}

void gui_draw_glyph(s32 x, s32 y, u16 colour, const u8 *glyph)
{
    gui_rect_t c;

    if (!glyph) return;

    s32 width = glyph[0];
    s32 height = glyph[1];
    const u8 *row = glyph + 2;

    if (!gui_clip(x, y, x + width - 1, y + height - 1, &c)) return;

    for (s32 gy = 0; gy < height; gy++, row += width) {
        for (s32 gx = 0; gx < width; gx++) {
            if (row[gx]) *pixel_at(x + gx, y + gy) = colour;
        }
    }
    g_display.dirty = true;
}

void gui_blit(s32 x, s32 y, const cm_image_t *img)
{
    gui_rect_t c;

    if (!img) return;
    if (!gui_clip(x, y, x + img->width - 1, y + img->height - 1, &c)) return;

    for (s32 row = 0; row < img->height; row++) {
        memcpy(pixel_at(x, y + row), img->pixels + row * img->width,
               (size_t)img->width * 2);
    }
    g_display.dirty = true;
}

cm_image_t *gui_capture(s32 x1, s32 y1, s32 x2, s32 y2, cm_image_t *reuse)
{
    gui_rect_t c;

    if (!gui_clip(x1, y1, x2, y2, &c)) return NULL;

    s32 width = c.x2 - c.x1 + 1;
    s32 height = c.y2 - c.y1 + 1;
    cm_image_t *img = reuse;

    if (!img || img->width != width || img->height != height) {
        img = calloc(1, sizeof(cm_image_t));
        if (!img) return NULL;
        img->width = width;
        img->height = height;
        img->data_size = width * height * 2;
        img->pixels = calloc(1, (size_t)img->data_size);
        if (!img->pixels) {
            free(img);
            return NULL;
        }
    }

    for (s32 row = 0; row < height; row++) {
        memcpy(img->pixels + row * width, pixel_at(c.x1, c.y1 + row),
               (size_t)width * 2);
    }

    /* The original copies the DDPIXELFORMAT of its display here. */
    img->bits_per_pixel = 32;
    img->bytes_per_pixel = 0x40;
    img->field_18 = 0;
    img->significant_bits = 16;
    img->r_mask = 0xf800;
    img->g_mask = CM_COLOUR_RGB565;
    img->b_mask = 0x001f;
    img->field_2c = 0;

    return img;
}

void gui_fade(s32 x1, s32 y1, s32 x2, s32 y2, s32 steps)
{
    gui_rect_t c;
    cm_image_t *img;
    u16 **shades;
    s32 frames = steps < 0 ? -steps : steps;

    if (steps == 0 || !gui_clip(x1, y1, x2, y2, &c)) return;
    img = gui_capture(c.x1, c.y1, c.x2, c.y2, NULL);
    if (!img) return;
    /* One row of shades for each colour in the picture, as in the original. */
    shades = calloc(0x10000, sizeof(*shades));
    if (!shades) {
        rgn_free(img);
        return;
    }

    for (s32 i = 0; i < img->width * img->height; i++) {
        u16 colour = img->pixels[i];

        if (shades[colour]) continue;
        shades[colour] = malloc(((size_t)frames + 1) * sizeof(u16));
        if (!shades[colour]) goto done;
        for (s32 k = 0; k <= frames; k++) {
            u16 shade = gui_scale_colour(colour, (u8)(k * 100 / frames));

            shades[colour][steps < 0 ? frames - k : k] = shade;
        }
    }

    for (s32 frame = 0; frame <= frames; frame++) {
        const u16 *src = img->pixels;

        for (s32 y = c.y1; y <= c.y2; y++) {
            u16 *dst = pixel_at(c.x1, y);

            for (s32 x = c.x1; x <= c.x2; x++) *dst++ = shades[*src++][frame];
        }
        display_flip(c.x1, c.y1, c.x2, c.y2);
    }

done:
    for (s32 i = 0; i < 0x10000; i++) free(shades[i]);
    free(shades);
    rgn_free(img);
}
