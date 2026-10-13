#include "gui_font.h"
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
