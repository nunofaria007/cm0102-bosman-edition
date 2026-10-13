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

/* Image header as stored in the game's pixel data. */
typedef struct {
    s32 width;
    s32 height;
    s32 reserved;
    u16 *pixels;
} cm_image_t;

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
