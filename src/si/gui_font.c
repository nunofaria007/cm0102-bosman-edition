#include "gui_font.h"
#include "display.h"
#include "gui_utils.h"
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

extern char *os_find_file_ci(const char *dir, const char *name);

gui_font_t g_fonts[GUI_FONT_COUNT];

static const char *const g_font_files[GUI_FONT_COUNT] = {
    NULL,
    "arial_narrow_10.fnt",
    "arial_narrow_11.fnt",
    "arial_14.fnt",
    "arial_16.fnt",
    "arial_18.fnt",
    "trade_cond_24_bold.fnt",
    "trade_cond_28_bold.fnt",
};

/* In fonts 6 and 7 the original gives these characters the glyph of another. */
static const u8 g_glyph_copies[][2] = {
    { 0x82, 0x2c }, { 0x84, 0x20 }, { 0x85, 0x20 }, { 0x86, 0x20 },
    { 0x87, 0x20 }, { 0x8b, 0x20 }, { 0x8c, 0xc6 }, { 0x9b, 0x20 },
    { 0x9c, 0xe6 }, { 0xa0, 0x9f }, { 0xd0, 0x44 }, { 0xde, 0x54 },
    { 0xf0, 0x64 }, { 0xfe, 0x74 },
};

static void clear_font(gui_font_t *font)
{
    for (s32 i = 0; i < GUI_FONT_CHARS; i++) {
        free(font->glyphs[i].rows);
    }
    memset(font, 0, sizeof(gui_font_t));
}

static bool read_glyph(FILE *f, gui_glyph_t *glyph, s32 height)
{
    s32 header[4];

    if (fread(header, sizeof(s32), 4, f) != 4) return false;
    glyph->width = header[0];
    glyph->row_bytes = header[1];
    glyph->left = header[2];
    glyph->right = header[3];
    if (glyph->row_bytes == 0) return true;
    if (glyph->width < 1 || glyph->row_bytes != (glyph->width + 1) / 2) {
        return false;
    }

    size_t size = (size_t)glyph->row_bytes * (size_t)height;

    glyph->rows = malloc(size);
    return glyph->rows && fread(glyph->rows, 1, size, f) == size;
}

static void copy_glyph(gui_font_t *font, u8 to, u8 from)
{
    gui_glyph_t *target = &font->glyphs[to];
    const gui_glyph_t *source = &font->glyphs[from];
    size_t size = (size_t)source->row_bytes * (size_t)font->height;

    free(target->rows);
    *target = *source;
    target->rows = NULL;
    if (size == 0) return;
    target->rows = malloc(size);
    if (target->rows) memcpy(target->rows, source->rows, size);
}

bool gui_font_load(s32 font, const char *path)
{
    gui_font_t *target = &g_fonts[font];
    FILE *f = path ? fopen(path, "rb") : NULL;
    bool ok;

    if (!f) return false;
    clear_font(target);
    ok = fread(&target->height, sizeof(s32), 1, f) == 1 &&
         target->height >= 1 && target->height <= 255;
    for (s32 c = GUI_FONT_FIRST_CHAR; ok && c < GUI_FONT_CHARS; c++) {
        ok = read_glyph(f, &target->glyphs[c], target->height);
    }
    fclose(f);

    if (!ok) {
        clear_font(target);
        return false;
    }
    if (font == 6 || font == 7) {
        size_t count = sizeof(g_glyph_copies) / sizeof(g_glyph_copies[0]);

        for (size_t i = 0; i < count; i++) {
            copy_glyph(target, g_glyph_copies[i][0], g_glyph_copies[i][1]);
        }
    }
    return true;
}

bool gui_fonts_load(const char *data_dir)
{
    for (s32 font = 1; font < GUI_FONT_COUNT; font++) {
        char *path = os_find_file_ci(data_dir, g_font_files[font]);
        bool ok = gui_font_load(font, path);

        free(path);
        if (!ok) return false;
    }
    return true;
}

#define MAX_CODES 4096

typedef struct {
    u32 r, g, b;
} rgb_t;

typedef struct {
    rgb_t text;
    rgb_t under;
    u32 last;
} blend_t;

/* FUN_005e5060 */
static void text_codes(u16 *codes, const char *text)
{
    size_t length = strlen(text);
    size_t count = 0;

    for (size_t i = 0; i < length && count < MAX_CODES - 1; i++) {
        const u8 *at = (const u8 *)text + i;
        u32 code = at[0];

        if (code == '#' && i + 5 < length && at[5] == '#' && isxdigit(at[1]) &&
            isxdigit(at[2]) && isxdigit(at[3]) && isxdigit(at[4])) {
            char digits[5] = { (char)at[1], (char)at[2], (char)at[3],
                               (char)at[4], '\0' };

            code = (u32)strtoul(digits, NULL, 16);
            i += 5;
        }
        codes[count++] = (u16)code;
    }
    codes[count] = 0;
}

static const gui_font_t *font_of(s16 font)
{
    return font >= 0 && font < GUI_FONT_COUNT ? &g_fonts[font] : NULL;
}

static u16 shown_code(u16 code)
{
    return code == '|' ? ' ' : code;
}

static const gui_glyph_t *glyph_of(const gui_font_t *font, u16 code)
{
    static const gui_glyph_t none;

    code = shown_code(code);
    return code < GUI_FONT_CHARS ? &font->glyphs[code] : &none;
}

/* FUN_005e5d60. The gap between codes[next - 1] and codes[next]. */
static s32 gap_before(const gui_font_t *font, const u16 *codes, s32 next)
{
    s32 gap;

    if (codes[next] == 0) return 0;
    gap = font->glyphs[' '].width * 3 / 4 - glyph_of(font, codes[next])->left -
          glyph_of(font, codes[next - 1])->right;
    return gap < 0 ? 0 : gap;
}

static bool has_width(u16 code)
{
    return code >= ' ';
}

s32 gui_line_width(s16 font, const char *text)
{
    const gui_font_t *from = font_of(font);
    u16 codes[MAX_CODES];
    s32 width = 0;

    if (!from) return 0;
    text_codes(codes, text);
    for (s32 i = 0; codes[i]; i++) {
        if (!has_width(codes[i])) continue;
        width += glyph_of(from, codes[i])->width + gap_before(from, codes, i + 1);
    }
    return width;
}

static rgb_t channels(u16 pixel)
{
    rgb_t rgb = {
        ((u32)(pixel & 0xf800) << 8) / 0xf801,
        ((u32)(pixel & 0x07e0) << 8) / 0x07e1,
        ((u32)(pixel & 0x001f) << 8) / 0x0020,
    };

    return rgb;
}

/* `level` is 1 to 14: the share of the text colour in fifteenths. */
static u16 blend_pixel(blend_t *blend, u16 under, u32 level)
{
    u32 rest = 15 - level;
    u32 r, g, b;
    u16 result;

    /* The original compares with the last pixel it wrote, not the last it read. */
    if (under != blend->last) blend->under = channels(under);
    r = (blend->text.r * level + blend->under.r * rest) / 15;
    g = (blend->text.g * level + blend->under.g * rest) / 15;
    b = (blend->text.b * level + blend->under.b * rest) / 15;
    result = (u16)((((r & 0xf8) << 5 | (g & 0xfc)) << 3) | b >> 3);
    blend->last = result;
    return result;
}

static void draw_glyph_rows(s32 x, s32 y, s32 height, const gui_glyph_t *glyph,
                            u16 colour, blend_t *blend)
{
    const u8 *row = glyph->rows;
    gui_rect_t c;

    if (!gui_clip(x, y, x + glyph->width - 1, y + height - 1, &c)) return;

    for (s32 gy = 0; gy < height; gy++, row += glyph->row_bytes) {
        for (s32 gx = 0; gx < glyph->width; gx++) {
            u32 level = gx & 1 ? row[gx / 2] & 0xf : row[gx / 2] >> 4;

            if (level == 15) {
                display_put_pixel(x + gx, y + gy, colour);
            } else if (level != 0) {
                u16 under = display_get_pixel(x + gx, y + gy);

                display_put_pixel(x + gx, y + gy,
                                  blend_pixel(blend, under, level));
            }
        }
    }
    g_display.dirty = true;
}

void gui_draw_line(s32 x, s32 y, s16 font, u16 colour, const char *text,
                   s32 cursor)
{
    const gui_font_t *from = font_of(font);
    blend_t blend = { channels(colour), { 0, 0, 0 }, 0xffffffff };
    u16 codes[MAX_CODES];
    s32 pen = x;
    s32 cursor_x = -1;
    s32 i;

    if (!from) return;
    text_codes(codes, text);
    for (i = 0; codes[i]; i++) {
        const gui_glyph_t *glyph = glyph_of(from, codes[i]);

        if (!has_width(codes[i])) continue;
        if (glyph->rows && shown_code(codes[i]) > ' ') {
            draw_glyph_rows(pen, y, from->height, glyph, colour, &blend);
        }
        if (i == cursor) cursor_x = x > pen - 1 ? x : pen - 1;
        pen += glyph->width + gap_before(from, codes, i + 1);
    }
    if (i == cursor) cursor_x = pen;
    if (cursor_x > -1) {
        gui_line(cursor_x, y, cursor_x, y + from->height - 1, GUI_LINE_SOLID,
                 colour);
    }
}
