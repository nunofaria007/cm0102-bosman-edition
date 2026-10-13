#ifndef CM_TYPES_H
#define CM_TYPES_H

#include <stdint.h>
#include <stdbool.h>

typedef uint8_t  u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef int8_t   s8;
typedef int16_t  s16;
typedef int32_t  s32;

/* Original game renders at 800x600 in 16-bit colour (RGB565). */
#define CM_INTERNAL_WIDTH  800
#define CM_INTERNAL_HEIGHT 600
#define CM_BPP             16

/*
 * RGN image header: 48 bytes on disk, matches a DirectDraw-style
 * BITMAPINFOHEADER with RGB565 colour masks.
 *
 * On disk the pixels pointer slot holds a placeholder.
 * The loader overwrites it with a pointer to the allocated pixel buffer.
 */
typedef struct {
    s32 width;
    s32 height;
    s32 data_size;
    u16 *pixels;
    s32 bits_per_pixel;
    s32 bytes_per_pixel;
    s32 field_18;
    s32 significant_bits;
    u32 r_mask;
    u32 g_mask;
    u32 b_mask;
    s32 field_2c;
} cm_image_t;

#define CM_RGN_HEADER_SIZE 48
#define CM_COLOUR_RGB565   0x07e0
#define CM_COLOUR_RGB555   0x03e0

/* Screen area / UI widget descriptor. */
typedef struct {
    s32 x1, y1, x2, y2;
    u32 flags;
    s32 cursor_pos;
    s32 max_len;
    s32 bg_image;
    char text[256];
} cm_area_t;

#endif
