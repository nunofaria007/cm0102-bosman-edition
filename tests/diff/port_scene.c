/*
 * The test scene of the port driver: a few areas and widgets that the
 * differential test fills field by field, in step with the same structs of
 * the original (tests/diff/scene.py).
 */
#include "port_scene.h"
#include "area.h"
#include "display.h"
#include "port_driver.h"
#include "gui_utils.h"
#include "scrman.h"
#include "scrman_input.h"
#include <SDL2/SDL.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define SCENE_AREAS   6
#define SCENE_WIDGETS 24

typedef struct {
    const char *name;
    size_t offset;
    size_t size;
    size_t count;
    bool is_signed;
} field_t;

#define SCALAR(type, member, is_signed) \
    { #member, offsetof(type, member), sizeof(((type *)0)->member), 1, is_signed }
#define ARRAY(type, member, is_signed) \
    { #member, offsetof(type, member), sizeof(((type *)0)->member[0]), \
      sizeof(((type *)0)->member) / sizeof(((type *)0)->member[0]), is_signed }

/* Same order as AREA_FIELDS in scene.py. */
static const field_t area_fields[] = {
    SCALAR(area_t, x1, true), SCALAR(area_t, y1, true),
    SCALAR(area_t, x2, true), SCALAR(area_t, y2, true),
    SCALAR(area_t, first_row, true), SCALAR(area_t, last_row, true),
    SCALAR(area_t, flags, false),
    ARRAY(area_t, column_x1, true), ARRAY(area_t, row_y1, true),
    ARRAY(area_t, column_x2, true), ARRAY(area_t, row_y2, true),
    SCALAR(area_t, scroll_to, true), SCALAR(area_t, colour, false),
    SCALAR(area_t, owner_widget, true),
    SCALAR(area_t, unknown_210, true), SCALAR(area_t, unknown_b78, true),
    ARRAY(area_t, widget_index, true),
    SCALAR(area_t, index, true), SCALAR(area_t, widget_count, true),
    SCALAR(area_t, first_visible, true),
    SCALAR(area_t, scrollbar.x1, true), SCALAR(area_t, scrollbar.y1, true),
    SCALAR(area_t, scrollbar.x2, true), SCALAR(area_t, scrollbar.y2, true),
    SCALAR(area_t, scrollbar.track_y1, true),
    SCALAR(area_t, scrollbar.track_y2, true),
    SCALAR(area_t, scrollbar.max_position, true),
    SCALAR(area_t, scrollbar.position, true),
    SCALAR(area_t, scrollbar.thumb_height, true),
    SCALAR(area_t, scrollbar.thumb_y1, true),
    SCALAR(area_t, scrollbar.thumb_y2, true),
    SCALAR(area_t, scrollbar.pressed, false),
    SCALAR(area_t, column_count, false), SCALAR(area_t, row_count, false),
    ARRAY(area_t, column_weight, false), ARRAY(area_t, row_weight, false),
    SCALAR(area_t, shown, true), SCALAR(area_t, has_scrollbar, true),
};

/* Same order as WIDGET_FIELDS in scene.py. */
static const field_t widget_fields[] = {
    SCALAR(widget_t, flags, false),
    SCALAR(widget_t, x1, true), SCALAR(widget_t, y1, true),
    SCALAR(widget_t, x2, true), SCALAR(widget_t, y2, true),
    SCALAR(widget_t, column, true), SCALAR(widget_t, row, true),
    SCALAR(widget_t, fixed_x1, true), SCALAR(widget_t, fixed_x2, true),
    SCALAR(widget_t, fixed_y1, true), SCALAR(widget_t, fixed_y2, true),
    SCALAR(widget_t, style, false), SCALAR(widget_t, colour, false),
    SCALAR(widget_t, area, true), SCALAR(widget_t, popup_area, true),
    SCALAR(widget_t, text_flags, false), SCALAR(widget_t, cursor, true),
    SCALAR(widget_t, highlight_colour, false),
    SCALAR(widget_t, font, true), SCALAR(widget_t, text_colour, false),
    ARRAY(widget_t, text, false),
    SCALAR(widget_t, lighten_once, true), SCALAR(widget_t, highlighted, true),
    SCALAR(widget_t, unknown_08, true), SCALAR(widget_t, text_limit, true),
    SCALAR(widget_t, unknown_48, false),
    SCALAR(widget_t, tick, true), SCALAR(widget_t, index, true),
    SCALAR(widget_t, unknown_7e, true), SCALAR(widget_t, text_changed, true),
};

/* Same order as MANAGER_FIELDS in scene.py. */
static const field_t manager_fields[] = {
    SCALAR(scrman_t, mode, true),
    SCALAR(scrman_t, area_slots, true), SCALAR(scrman_t, widget_slots, true),
    ARRAY(scrman_t, area_order, true), ARRAY(scrman_t, widget_order, true),
    SCALAR(scrman_t, area_count, true), SCALAR(scrman_t, widget_count, true),
    SCALAR(scrman_t, darken_screen, true),
    SCALAR(scrman_t, unknown_12f4fe, false), SCALAR(scrman_t, unknown_12f4ff, true),
    SCALAR(scrman_t, unknown_12f501, true), SCALAR(scrman_t, unknown_12f503, true),
    SCALAR(scrman_t, unknown_12f505, true), ARRAY(scrman_t, unknown_12f509, true),
    SCALAR(scrman_t, unknown_12f52d, true), SCALAR(scrman_t, unknown_12f52f, true),
    SCALAR(scrman_t, unknown_12f531, true), SCALAR(scrman_t, passed_focus, true),
    SCALAR(scrman_t, fade_in, true),
    SCALAR(scrman_t, unknown_12f539, true), SCALAR(scrman_t, unknown_12f53b, true),
    SCALAR(scrman_t, skip_draw, true), SCALAR(scrman_t, unknown_12f541, false),
    SCALAR(scrman_t, laid_out, true),
    SCALAR(scrman_t, widget_overflow, true), SCALAR(scrman_t, area_overflow, true),
    SCALAR(scrman_t, unknown_12f551, true), SCALAR(scrman_t, unknown_12f555, true),
    ARRAY(scrman_t, background_name, false),
};

#define COUNT(table) (sizeof(table) / sizeof((table)[0]))

static area_t g_areas[SCENE_AREAS];
static widget_t g_widgets[SCENE_WIDGETS];
static scrman_t g_manager;

extern u32 rgn_free_count;

static void reset(void)
{
    memset(g_areas, 0, sizeof(g_areas));
    memset(g_widgets, 0, sizeof(g_widgets));
    for (s32 i = 0; i < SCENE_AREAS; i++) {
        g_areas[i].areas = g_areas;
        g_areas[i].widgets = g_widgets;
    }
    for (s32 i = 0; i < SCENE_WIDGETS; i++) {
        g_widgets[i].areas = g_areas;
    }
}

/* The tables hold fields of 1, 2 and 4 bytes, and intptr_t. */
static long long read_value(const void *at, const field_t *field)
{
    if (field->size == sizeof(intptr_t)) return *(const intptr_t *)at;
    if (field->size == 1) return *(const u8 *)at;
    if (field->size == 2 && field->is_signed) return *(const s16 *)at;
    if (field->size == 2) return *(const u16 *)at;
    if (field->is_signed) return *(const s32 *)at;
    return *(const u32 *)at;
}

static void write_value(void *at, const field_t *field, long long value)
{
    if (field->size == sizeof(intptr_t)) {
        *(intptr_t *)at = (intptr_t)value;
    } else if (field->size == 1) {
        *(u8 *)at = (u8)value;
    } else if (field->size == 2) {
        *(u16 *)at = (u16)value;
    } else {
        *(u32 *)at = (u32)value;
    }
}

/* Picks the field table and the struct for "area 2" or "widget 5". */
static bool select_struct(const char **args, const field_t **fields,
                          size_t *field_count, void **base)
{
    char kind[16];
    int index, used;

    if (sscanf(*args, "%15s %d%n", kind, &index, &used) != 2) return false;
    *args += used;
    if (!strcmp(kind, "manager") && index == 0) {
        *fields = manager_fields;
        *field_count = COUNT(manager_fields);
        *base = &g_manager;
        return true;
    }
    if (!strcmp(kind, "manager_area") && index >= 0 &&
        index < SCRMAN_MAX_AREAS) {
        *fields = area_fields;
        *field_count = COUNT(area_fields);
        *base = &g_manager.areas[index];
        return true;
    }
    if (!strcmp(kind, "manager_widget") && index >= 0 &&
        index < SCRMAN_MAX_WIDGETS) {
        *fields = widget_fields;
        *field_count = COUNT(widget_fields);
        *base = &g_manager.widgets[index];
        return true;
    }
    if (!strcmp(kind, "area") && index >= 0 && index < SCENE_AREAS) {
        *fields = area_fields;
        *field_count = COUNT(area_fields);
        *base = &g_areas[index];
        return true;
    }
    if (!strcmp(kind, "widget") && index >= 0 && index < SCENE_WIDGETS) {
        *fields = widget_fields;
        *field_count = COUNT(widget_fields);
        *base = &g_widgets[index];
        return true;
    }
    return false;
}

/* "set area 2 column_weight 1 2 1": writes the values from the field start. */
static bool set_field(const char *args)
{
    const field_t *fields;
    size_t field_count;
    void *base;
    char name[40];
    int used;

    if (!select_struct(&args, &fields, &field_count, &base)) return false;
    if (sscanf(args, "%39s%n", name, &used) != 1) return false;
    args += used;

    for (size_t f = 0; f < field_count; f++) {
        if (strcmp(fields[f].name, name)) continue;
        for (size_t i = 0; i < fields[f].count; i++) {
            char *end;
            long long value = strtoll(args, &end, 10);

            if (end == args) return i > 0;
            args = end;
            write_value((char *)base + fields[f].offset + i * fields[f].size,
                        &fields[f], value);
        }
        return true;
    }
    return false;
}

/* "get area 2": every value of every field, in table order. */
static bool get_fields(const char *args)
{
    const field_t *fields;
    size_t field_count;
    void *base;

    if (!select_struct(&args, &fields, &field_count, &base)) return false;
    for (size_t f = 0; f < field_count; f++) {
        for (size_t i = 0; i < fields[f].count; i++) {
            const char *at = (char *)base + fields[f].offset;
            printf("%lld ", read_value(at + i * fields[f].size, &fields[f]));
        }
    }
    printf("\n");
    return true;
}

/* "fields area": the field names, so the test can check its own table. */
static bool list_fields(const char *args)
{
    const field_t *fields = widget_fields;
    size_t count = COUNT(widget_fields);

    if (strstr(args, "area")) {
        fields = area_fields;
        count = COUNT(area_fields);
    } else if (strstr(args, "manager")) {
        fields = manager_fields;
        count = COUNT(manager_fields);
    }

    for (size_t f = 0; f < count; f++) {
        printf("%s:%zu ", fields[f].name, fields[f].count);
    }
    printf("\n");
    return true;
}

/* Finds the image pointer that "widget 3 saved" or "area 1 scrollbar" names. */
static cm_image_t **select_image(const char **args)
{
    char kind[16], which[16];
    int index, used;

    if (sscanf(*args, "%15s %d %15s%n", kind, &index, which, &used) != 3) {
        return NULL;
    }
    *args += used;
    if (!strcmp(kind, "manager") && index == 0) {
        if (!strcmp(which, "unknown_12f521")) return &g_manager.unknown_12f521;
        if (!strcmp(which, "background")) return &g_manager.background;
    }
    if (!strcmp(kind, "manager_widget") && index >= 0 &&
        index < SCRMAN_MAX_WIDGETS) {
        widget_t *widget = &g_manager.widgets[index];

        if (!strcmp(which, "saved")) return &widget->saved;
        if (!strcmp(which, "picture")) return &widget->picture;
    }
    if (!strcmp(kind, "manager_area") && index >= 0 &&
        index < SCRMAN_MAX_AREAS) {
        area_t *area = &g_manager.areas[index];

        if (!strcmp(which, "saved")) return &area->saved;
        if (!strcmp(which, "scrollbar")) return &area->scrollbar.saved;
    }
    if (!strcmp(kind, "widget") && index >= 0 && index < SCENE_WIDGETS) {
        if (!strcmp(which, "saved")) return &g_widgets[index].saved;
        if (!strcmp(which, "picture")) return &g_widgets[index].picture;
    }
    if (!strcmp(kind, "area") && index >= 0 && index < SCENE_AREAS) {
        if (!strcmp(which, "saved")) return &g_areas[index].saved;
        if (!strcmp(which, "scrollbar")) return &g_areas[index].scrollbar.saved;
    }
    return NULL;
}

/* "image widget 3 saved": one image that a struct of the scene points to. */
static bool show_image(const char *args)
{
    cm_image_t **image = select_image(&args);

    if (!image) return false;
    print_image(*image);
    return true;
}

/* "capture_to area 1 saved 0 0 9 9": points the struct at a new capture. */
static bool capture_to(const char *args)
{
    cm_image_t **image = select_image(&args);
    int x1, y1, x2, y2;

    if (!image) return false;
    if (sscanf(args, "%d %d %d %d", &x1, &y1, &x2, &y2) != 4) return false;
    *image = gui_capture(x1, y1, x2, y2, NULL);
    return true;
}

/* Reads "-1" for no table, or a count and that many weights. */
static bool read_weights(const char **args, u8 *table, const u8 **weights)
{
    char *end;
    long count = strtol(*args, &end, 10);

    if (end == *args || count > 255) return false;
    *args = end;
    *weights = count < 0 ? NULL : table;
    for (long i = 0; i < count; i++) {
        long weight = strtol(*args, &end, 10);

        if (end == *args) return false;
        *args = end;
        table[i] = (u8)weight;
    }
    return true;
}

/* "manager_area x1 y1 x2 y2 columns rows flags colour owner", then two weight tables. */
static bool manager_area(const char *args)
{
    static u8 column_table[256], row_table[256];
    long long v[9];
    int used;

    if (sscanf(args, "%lld %lld %lld %lld %lld %lld %lld %lld %lld%n", &v[0],
               &v[1], &v[2], &v[3], &v[4], &v[5], &v[6], &v[7], &v[8],
               &used) != 9) {
        return false;
    }
    args += used;
    const u8 *columns, *rows;

    if (!read_weights(&args, column_table, &columns)) return false;
    if (!read_weights(&args, row_table, &rows)) return false;

    printf("%d\n", scrman_area_create(&g_manager, (s32)v[0], (s32)v[1],
                                      (s32)v[2], (s32)v[3], (u8)v[4], columns,
                                      (u8)v[5], rows, (u32)v[6], (u16)v[7],
                                      (s16)v[8]));
    return true;
}

/*
 * "manager_widget" with 17 numbers in the order of scrman_widget_create, then
 * the text as "null" or "-" and two hex digits for each byte.
 */
static bool manager_widget(const char *args)
{
    static char text[1024];
    long long v[17];
    const char *hex;
    size_t length = 0;
    unsigned value;
    char *end;

    for (int i = 0; i < 17; i++) {
        v[i] = strtoll(args, &end, 10);
        if (end == args) return false;
        args = end;
    }
    while (*args == ' ') args++;
    if (*args != '-' && strncmp(args, "null", 4)) return false;
    for (hex = args + 1; *args == '-' && sscanf(hex, "%2x", &value) == 1;
         hex += 2) {
        if (length == sizeof(text) - 1) return false;
        text[length++] = (char)value;
    }
    text[length] = '\0';

    printf("%d\n", scrman_widget_create(
        &g_manager, (u32)v[0], (s32)v[1], (s32)v[2], (s32)v[3], (s32)v[4],
        (s32)v[5], (s32)v[6], (u32)v[7], (u16)v[8], (u16)v[9], (u32)v[10],
        (s16)v[11], (u16)v[12], *args == '-' ? text : NULL, (s32)v[13],
        (s32)v[14], (intptr_t)v[15], (s16)v[16]));
    return true;
}

#define MAX_FLIPS 64

/* The flips since the last "flips" command, each with the screen at that time. */
static struct {
    s32 box[4];
    u16 *screen;
} g_flips[MAX_FLIPS];
static int g_flip_count;

static void record_flip(s32 x1, s32 y1, s32 x2, s32 y2)
{
    size_t size = CM_INTERNAL_WIDTH * CM_INTERNAL_HEIGHT * sizeof(u16);

    if (g_flip_count == MAX_FLIPS) return;
    g_flips[g_flip_count].box[0] = x1;
    g_flips[g_flip_count].box[1] = y1;
    g_flips[g_flip_count].box[2] = x2;
    g_flips[g_flip_count].box[3] = y2;
    g_flips[g_flip_count].screen = malloc(size);
    if (g_flips[g_flip_count].screen) {
        memcpy(g_flips[g_flip_count].screen, g_display.framebuffer, size);
    }
    g_flip_count++;
}

void scene_record_flips(void)
{
    display_on_flip = record_flip;
}

/* "flips": the count, then a line and the screen for each flip. */
static void show_flips(void)
{
    size_t size = CM_INTERNAL_WIDTH * CM_INTERNAL_HEIGHT * sizeof(u16);

    printf("%d\n", g_flip_count);
    for (int i = 0; i < g_flip_count; i++) {
        printf("%d %d %d %d %d\n", g_flips[i].box[0], g_flips[i].box[1],
               g_flips[i].box[2], g_flips[i].box[3],
               g_flips[i].screen != NULL);
        if (g_flips[i].screen) fwrite(g_flips[i].screen, 1, size, stdout);
        free(g_flips[i].screen);
    }
    g_flip_count = 0;
}

/* "manager_screen x1 y1 x2 y2 -hex": the background name follows the "-". */
static bool manager_screen(const char *args)
{
    static char name[300];
    long long v[4];
    size_t length = 0;
    unsigned value;
    char *end;
    const char *hex;

    for (int i = 0; i < 4; i++) {
        v[i] = strtoll(args, &end, 10);
        if (end == args) return false;
        args = end;
    }
    while (*args == ' ') args++;
    if (*args != '-') return false;
    for (hex = args + 1; sscanf(hex, "%2x", &value) == 1; hex += 2) {
        if (length == sizeof(name) - 1) return false;
        name[length++] = (char)value;
    }
    name[length] = '\0';
    scrman_draw(&g_manager, name, (s16)v[0], (s16)v[1], (s16)v[2],
                (s16)v[3]);
    return true;
}

extern void input_poll(SDL_Event *e);

/*
 * The SDL key for a Windows virtual key code, from a table of its own, so
 * that the test checks the table of input.c.
 */
static SDL_Keycode sdl_key(long vk)
{
    static const struct {
        long vk;
        SDL_Keycode key;
    } keys[] = {
        {0x08, SDLK_BACKSPACE}, {0x09, SDLK_TAB}, {0x0d, SDLK_RETURN},
        {0x10, SDLK_LSHIFT}, {0x11, SDLK_LCTRL}, {0x14, SDLK_CAPSLOCK},
        {0x1b, SDLK_ESCAPE}, {0x20, SDLK_SPACE}, {0x21, SDLK_PAGEUP},
        {0x22, SDLK_PAGEDOWN}, {0x23, SDLK_END}, {0x24, SDLK_HOME},
        {0x25, SDLK_LEFT}, {0x26, SDLK_UP}, {0x27, SDLK_RIGHT},
        {0x28, SDLK_DOWN}, {0x2d, SDLK_INSERT}, {0x2e, SDLK_DELETE},
        {0x60, SDLK_KP_0}, {0x61, SDLK_KP_1}, {0x62, SDLK_KP_2},
        {0x63, SDLK_KP_3}, {0x64, SDLK_KP_4}, {0x65, SDLK_KP_5},
        {0x66, SDLK_KP_6}, {0x67, SDLK_KP_7}, {0x68, SDLK_KP_8},
        {0x69, SDLK_KP_9}, {0x6a, SDLK_KP_MULTIPLY}, {0x6b, SDLK_KP_PLUS},
        {0x6d, SDLK_KP_MINUS}, {0x6e, SDLK_KP_PERIOD},
        {0x6f, SDLK_KP_DIVIDE}, {0x70, SDLK_F1}, {0x71, SDLK_F2},
        {0x72, SDLK_F3}, {0x73, SDLK_F4}, {0x74, SDLK_F5}, {0x75, SDLK_F6},
        {0x76, SDLK_F7}, {0x77, SDLK_F8}, {0x78, SDLK_F9}, {0x7a, SDLK_F11},
        {0x7b, SDLK_F12}, {0xba, SDLK_SEMICOLON}, {0xbb, SDLK_EQUALS},
        {0xbc, SDLK_COMMA}, {0xbd, SDLK_MINUS}, {0xbe, SDLK_PERIOD},
        {0xbf, SDLK_SLASH}, {0xc0, SDLK_BACKQUOTE}, {0xdb, SDLK_LEFTBRACKET},
        {0xdc, SDLK_BACKSLASH}, {0xdd, SDLK_RIGHTBRACKET},
        {0xde, SDLK_QUOTE},
    };

    if (vk >= 0x30 && vk <= 0x39) return SDLK_0 + (SDL_Keycode)(vk - 0x30);
    if (vk >= 0x41 && vk <= 0x5a) return SDLK_a + (SDL_Keycode)(vk - 0x41);
    for (size_t i = 0; i < sizeof(keys) / sizeof(keys[0]); i++) {
        if (keys[i].vk == vk) return keys[i].key;
    }
    return SDLK_UNKNOWN;
}

/*
 * "sdl kind a b c": one SDL event into input_poll, the same one that the
 * test gives the window procedure of the original as a Windows message.
 * kind: 0 key down (a = virtual key), 1 wheel (a = notches), 2 mouse move
 * (a, b = x, y), 3 button (a = 1 down or 0 up, b = 1 left or 3 right, c = x
 * packed with y as y * 65536 + x), 4 text (a = a Latin-1 character, sent as
 * UTF-8). Prints the mouse globals, the key code and the character.
 */
static bool sdl_event(const char *args)
{
    long v[4] = {0};
    SDL_Event e;

    if (sscanf(args, "%ld %ld %ld %ld", &v[0], &v[1], &v[2], &v[3]) < 2) {
        return false;
    }
    memset(&e, 0, sizeof(e));
    if (v[0] == 0) {
        e.type = SDL_KEYDOWN;
        e.key.keysym.sym = sdl_key(v[1]);
    } else if (v[0] == 1) {
        e.type = SDL_MOUSEWHEEL;
        e.wheel.y = (Sint32)v[1];
    } else if (v[0] == 2) {
        e.type = SDL_MOUSEMOTION;
        e.motion.x = (Sint32)v[1];
        e.motion.y = (Sint32)v[2];
    } else if (v[0] == 4) {
        e.type = SDL_TEXTINPUT;
        if (v[1] < 0x80) {
            e.text.text[0] = (char)v[1];
        } else {
            e.text.text[0] = (char)(0xc0 | (v[1] >> 6));
            e.text.text[1] = (char)(0x80 | (v[1] & 0x3f));
        }
    } else {
        e.type = v[1] ? SDL_MOUSEBUTTONDOWN : SDL_MOUSEBUTTONUP;
        e.button.button = (Uint8)v[2];
        e.button.x = (Sint32)(v[3] & 0xffff);
        e.button.y = (Sint32)(v[3] >> 16);
    }
    input_poll(&e);
    printf("%d %d %d %d %d %d %d %d %d\n", g_mouse_x, g_mouse_y, g_left_held,
           g_right_held, g_left_clicked, g_right_clicked, g_click_frames,
           g_key_code, g_key_char);
    return true;
}

bool scene_command(const char *cmd, const char *args)
{
    int index = (int)strtol(args, NULL, 10);
    int second = 0;

    sscanf(args, "%*d %d", &second);

    if (!strcmp(cmd, "scene_reset")) {
        reset();
        return true;
    }
    if (!strcmp(cmd, "manager_clear")) {
        memset(&g_manager, 0, sizeof(g_manager));
        return true;
    }
    if (!strcmp(cmd, "manager_new")) {
        scrman_init(&g_manager, (u32)strtoll(args, NULL, 10));
        return true;
    }
    if (!strcmp(cmd, "manager_reset")) {
        scrman_reset(&g_manager, index != 0);
        return true;
    }
    if (!strcmp(cmd, "manager_area")) return manager_area(args);
    if (!strcmp(cmd, "manager_widget")) return manager_widget(args);
    if (!strcmp(cmd, "manager_wlinks")) {
        if (index < 0 || index >= SCRMAN_MAX_WIDGETS) return false;
        const widget_t *widget = &g_manager.widgets[index];

        printf("%d %d %d\n", widget->areas == g_manager.areas,
               widget->widgets == g_manager.widgets,
               widget->text_source != NULL);
        return true;
    }
    if (!strcmp(cmd, "manager_draw")) {
        if (index < 0 || index >= SCRMAN_MAX_WIDGETS) return false;
        widget_draw(&g_manager.widgets[index], second != 0);
        return true;
    }
    if (!strcmp(cmd, "manager_screen")) {
        display_on_flip = record_flip;
        return manager_screen(args);
    }
    if (!strcmp(cmd, "fade")) {
        long long v[5] = {0};

        if (sscanf(args, "%lld %lld %lld %lld %lld", &v[0], &v[1], &v[2],
                   &v[3], &v[4]) != 5) {
            return false;
        }
        display_on_flip = record_flip;
        gui_fade((s32)v[0], (s32)v[1], (s32)v[2], (s32)v[3], (s32)v[4]);
        return true;
    }
    if (!strcmp(cmd, "flips")) {
        show_flips();
        return true;
    }
    if (!strcmp(cmd, "data_dir")) {
        char path[1024];

        if (sscanf(args, "%1023s", path) != 1) return false;
        g_data_dir = strdup(path);
        return true;
    }
    if (!strcmp(cmd, "draw_globals")) {
        g_no_backgrounds = index;
        g_darken_screens = second;
        return true;
    }
    if (!strcmp(cmd, "mouse")) {
        long long v[5];

        if (sscanf(args, "%lld %lld %lld %lld %lld", &v[0], &v[1], &v[2],
                   &v[3], &v[4]) != 5) {
            return false;
        }
        g_mouse_x = (s32)v[0];
        g_mouse_y = (s32)v[1];
        g_left_held = (s32)v[2];
        g_right_held = (s32)v[3];
        g_cursor_kind = (s32)v[4];
        return true;
    }
    if (!strcmp(cmd, "hit")) {
        display_on_flip = record_flip;
        printf("%d %d %d\n", scrman_hit_test(&g_manager), g_cursor_kind,
               g_left_held);
        return true;
    }
    if (!strcmp(cmd, "sdl")) return sdl_event(args);
    if (!strcmp(cmd, "clicks")) {
        /* "clicks left right": the one-shot click flags of the original. */
        g_left_clicked = index;
        g_right_clicked = second;
        return true;
    }
    if (!strcmp(cmd, "tick")) {
        g_tick = index;
        return true;
    }
    if (!strcmp(cmd, "manager_links")) {
        if (index < 0 || index >= SCRMAN_MAX_AREAS) return false;
        const area_t *area = &g_manager.areas[index];

        printf("%d %d\n", area->areas == g_manager.areas,
               area->widgets == g_manager.widgets);
        return true;
    }
    if (!strcmp(cmd, "manager_pointer")) {
        /* "manager_pointer 1" sets unknown_12f529. Prints whether it is set. */
        if (index) g_manager.unknown_12f529 = &g_manager;
        printf("%d\n", g_manager.unknown_12f529 != NULL);
        return true;
    }
    if (!strcmp(cmd, "frees")) {
        printf("%u\n", rgn_free_count);
        rgn_free_count = 0;
        return true;
    }
    if (!strcmp(cmd, "fields")) return list_fields(args);
    if (!strcmp(cmd, "set")) return set_field(args);
    if (!strcmp(cmd, "get")) return get_fields(args);
    if (!strcmp(cmd, "image")) return show_image(args);
    if (!strcmp(cmd, "capture_to")) return capture_to(args);
    if (!strcmp(cmd, "draw_widget")) {
        if (index < 0 || index >= SCENE_WIDGETS) return false;
        widget_draw(&g_widgets[index], second != 0);
        return true;
    }
    if (!strcmp(cmd, "same_picture")) {
        if (index < 0 || index >= SCENE_WIDGETS) return false;
        if (second < 0 || second >= SCENE_WIDGETS) return false;
        printf("%d\n", g_widgets[index].picture == g_widgets[second].picture);
        return true;
    }
    if (index < 0 || index >= SCENE_AREAS) return false;

    if (!strcmp(cmd, "layout")) {
        area_layout(&g_areas[index]);
        return true;
    }
    if (!strcmp(cmd, "draw_area")) {
        area_draw(&g_areas[index], second != 0);
        return true;
    }
    if (!strcmp(cmd, "release_area")) {
        area_release(&g_areas[index]);
        return true;
    }
    if (!strcmp(cmd, "popup")) {
        s32 x = -9999, y = -9999;

        area_popup_position(&g_areas[index], &x, &y);
        printf("%d %d\n", x, y);
        return true;
    }
    return false;
}

scrman_t *scene_manager(void)
{
    return &g_manager;
}
