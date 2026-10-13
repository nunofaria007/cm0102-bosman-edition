#include "cm_types.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/*
 * RGN is SI's proprietary image format.
 * Header layout (from RE of FUN_005e3fd0 and related blitters):
 *   offset 0x00: u32 width
 *   offset 0x04: u32 height
 *   offset 0x08: u32 reserved (often 0)
 *   offset 0x0c: pixel data, row-major, RGB565 16-bit
 *
 * TODO: Verify with REA whether compression or palette variants exist
 *       in the shipped data files.
 */

cm_image_t *rgn_load(const char *path)
{
    FILE *f = fopen(path, "rb");
    if (!f) return NULL;

    u32 header[3];
    if (fread(header, sizeof(u32), 3, f) != 3) {
        fclose(f);
        return NULL;
    }

    s32 w = (s32)header[0];
    s32 h = (s32)header[1];

    if (w <= 0 || w > 4096 || h <= 0 || h > 4096) {
        fclose(f);
        return NULL;
    }

    u32 pixel_count = (u32)(w * h);
    u16 *pixels = malloc(pixel_count * sizeof(u16));
    if (!pixels) {
        fclose(f);
        return NULL;
    }

    if (fread(pixels, sizeof(u16), pixel_count, f) != pixel_count) {
        free(pixels);
        fclose(f);
        return NULL;
    }
    fclose(f);

    cm_image_t *img = malloc(sizeof(cm_image_t));
    if (!img) {
        free(pixels);
        return NULL;
    }

    img->width    = w;
    img->height   = h;
    img->reserved = (s32)header[2];
    img->pixels   = pixels;

    return img;
}

void rgn_free(cm_image_t *img)
{
    if (!img) return;
    free(img->pixels);
    free(img);
}
