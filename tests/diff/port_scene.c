/*
 * The test scene of the port driver: a few areas and widgets that the
 * differential test fills field by field, in step with the same structs of
 * the original (tests/diff/scene.py).
 */
#include "port_scene.h"
#include "area.h"
#include "port_driver.h"
#include "gui_utils.h"
#include "scrman.h"
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
    SCALAR(area_t, unknown_210, true), ARRAY(area_t, widget_index, true),
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
};

/* Same order as MANAGER_FIELDS in scene.py. */
static const field_t manager_fields[] = {
    SCALAR(scrman_t, mode, true),
    SCALAR(scrman_t, area_slots, true), SCALAR(scrman_t, widget_slots, true),
    ARRAY(scrman_t, area_order, true), ARRAY(scrman_t, widget_order, true),
    SCALAR(scrman_t, area_count, true), SCALAR(scrman_t, widget_count, true),
    SCALAR(scrman_t, clear_screen, true),
    SCALAR(scrman_t, unknown_12f4fe, false), SCALAR(scrman_t, unknown_12f4ff, true),
    SCALAR(scrman_t, unknown_12f501, true), SCALAR(scrman_t, unknown_12f503, true),
    SCALAR(scrman_t, unknown_12f505, true), ARRAY(scrman_t, unknown_12f509, true),
    SCALAR(scrman_t, unknown_12f52d, true), SCALAR(scrman_t, unknown_12f52f, true),
    SCALAR(scrman_t, unknown_12f531, true), SCALAR(scrman_t, unknown_12f533, true),
    SCALAR(scrman_t, darken_once, true),
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

/* The tables hold fields of 1, 2 and 4 bytes only. */
static long long read_value(const void *at, const field_t *field)
{
    if (field->size == 1) return *(const u8 *)at;
    if (field->size == 2 && field->is_signed) return *(const s16 *)at;
    if (field->size == 2) return *(const u16 *)at;
    if (field->is_signed) return *(const s32 *)at;
    return *(const u32 *)at;
}

static void write_value(void *at, const field_t *field, long long value)
{
    if (field->size == 1) {
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
static const u8 *read_weights(const char **args, u8 *table)
{
    char *end;
    long count = strtol(*args, &end, 10);

    *args = end;
    if (count < 0) return NULL;
    for (long i = 0; i < count; i++) {
        long weight = strtol(*args, &end, 10);

        *args = end;
        if (i < 256) table[i] = (u8)weight;
    }
    return table;
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
    const u8 *columns = read_weights(&args, column_table);
    const u8 *rows = read_weights(&args, row_table);

    printf("%d\n", scrman_area_create(&g_manager, (s32)v[0], (s32)v[1],
                                      (s32)v[2], (s32)v[3], (u8)v[4], columns,
                                      (u8)v[5], rows, (u32)v[6], (u16)v[7],
                                      (s16)v[8]));
    return true;
}

bool scene_command(const char *cmd, const char *args)
{
    int index = atoi(args);
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
