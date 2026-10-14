#include "cm_types.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/*
 * RGN is SI's image format for CM 01/02.
 *
 * File layout (verified against shipped .RGN files and FUN_005e40d0):
 *   48-byte header (12 x s32), then raw 16-bit pixel data.
 *
 * Header fields:
 *   [0]  width
 *   [1]  height
 *   [2]  data_size       (width * height * 2)
 *   [3]  placeholder     (overwritten at runtime with pixel pointer)
 *   [4]  bits_per_pixel  (32)
 *   [5]  bytes_per_pixel (64)
 *   [6]  unused          (0)
 *   [7]  significant_bits(16)
 *   [8]  r_mask          (0xf800 for RGB565)
 *   [9]  g_mask / colour format (0x07e0 = RGB565, 0x03e0 = RGB555)
 *   [10] b_mask          (0x001f)
 *   [11] unused          (0)
 *
 * Pixel data: row-major, 16-bit, either RGB565 or RGB555.
 * The loader converts to RGB565 whenever the green mask of the file is not
 * that of RGB565, as the original does for an RGB565 display.
 */

static void convert_rgb555_to_rgb565(u16 *pixels, s32 count)
{
    for (s32 i = 0; i < count; i++) {
        u16 p = pixels[i];
        pixels[i] = (u16)(((p & 0xffe0) << 1) | (p & 0x1f));
    }
}

cm_image_t *rgn_load_into(const char *path, cm_image_t *reuse);

cm_image_t *rgn_load(const char *path)
{
    return rgn_load_into(path, NULL);
}

/*
 * FUN_005e40d0. Reads the pixels into `reuse` when it has the same width and
 * height, and returns it. The original then keeps the other header fields of
 * `reuse`. When that read fails, `reuse` is not freed.
 */
cm_image_t *rgn_load_into(const char *path, cm_image_t *reuse)
{
    if (!path) return NULL;
    FILE *f = fopen(path, "rb");
    if (!f) return NULL;

    s32 raw_header[12];
    if (fread(raw_header, 4, 12, f) != 12) {
        fclose(f);
        return NULL;
    }

    s32 w         = raw_header[0];
    s32 h         = raw_header[1];
    s32 data_size = raw_header[2];

    if (w <= 0 || w > 4096 || h <= 0 || h > 4096) {
        fclose(f);
        return NULL;
    }

    /* The original trusts data_size. Every shipped file has exactly this size. */
    if (data_size != w * h * 2) {
        fclose(f);
        return NULL;
    }

    if (reuse && reuse->width == w && reuse->height == h) {
        bool read = reuse->data_size == data_size &&
                    fread(reuse->pixels, 1, (size_t)data_size, f) ==
                        (size_t)data_size;

        fclose(f);
        if (!read) return NULL;
        if ((u32)raw_header[9] != CM_COLOUR_RGB565) {
            convert_rgb555_to_rgb565(reuse->pixels, w * h);
        }
        return reuse;
    }

    u16 *pixels = malloc((size_t)data_size);
    if (!pixels) {
        fclose(f);
        return NULL;
    }

    if (fread(pixels, 1, (size_t)data_size, f) != (size_t)data_size) {
        free(pixels);
        fclose(f);
        return NULL;
    }
    fclose(f);

    u32 g_mask = (u32)raw_header[9];
    if (g_mask != CM_COLOUR_RGB565) {
        convert_rgb555_to_rgb565(pixels, w * h);
        g_mask = CM_COLOUR_RGB565;
    }

    cm_image_t *img = calloc(1, sizeof(cm_image_t));
    if (!img) {
        free(pixels);
        return NULL;
    }

    img->width           = w;
    img->height          = h;
    img->data_size       = data_size;
    img->pixels          = pixels;
    img->bits_per_pixel  = raw_header[4];
    img->bytes_per_pixel = raw_header[5];
    img->field_18        = raw_header[6];
    img->significant_bits= raw_header[7];
    img->r_mask          = (u32)raw_header[8];
    img->g_mask          = g_mask;
    img->b_mask          = (u32)raw_header[10];
    img->field_2c        = raw_header[11];

    return img;
}

/* Counts the images freed, for the differential test. */
u32 rgn_free_count;

void rgn_free(cm_image_t *img)
{
    if (!img) return;
    rgn_free_count++;
    free(img->pixels);
    free(img);
}
