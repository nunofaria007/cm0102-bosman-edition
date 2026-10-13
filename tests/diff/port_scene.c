/*
 * The test scene of the port driver: a few areas and widgets that the
 * differential test fills field by field, in step with the same structs of
 * the original (tests/diff/scene.py).
 */
#include "port_scene.h"
#include "area.h"
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
    SCALAR(area_t, row_offset, true), SCALAR(area_t, last_row, true),
    SCALAR(area_t, flags, false),
    ARRAY(area_t, column_x1, true), ARRAY(area_t, row_y1, true),
    ARRAY(area_t, column_x2, true), ARRAY(area_t, row_y2, true),
    SCALAR(area_t, scroll_to, true), SCALAR(area_t, colour, false),
    SCALAR(area_t, owner_widget, true), ARRAY(area_t, widget_index, true),
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
};

#define COUNT(table) (sizeof(table) / sizeof((table)[0]))

static area_t areas[SCENE_AREAS];
static widget_t widgets[SCENE_WIDGETS];

static void reset(void)
{
    memset(areas, 0, sizeof(areas));
    memset(widgets, 0, sizeof(widgets));
    for (int i = 0; i < SCENE_AREAS; i++) {
        areas[i].areas = areas;
        areas[i].widgets = widgets;
    }
    for (int i = 0; i < SCENE_WIDGETS; i++) widgets[i].areas = areas;
}

static long long read_value(const void *at, const field_t *field)
{
    switch (field->size + (field->is_signed ? 100 : 0)) {
    case 1:   return *(const u8 *)at;
    case 2:   return *(const u16 *)at;
    case 4:   return *(const u32 *)at;
    case 102: return *(const s16 *)at;
    default:  return *(const s32 *)at;
    }
}

static void write_value(void *at, const field_t *field, long long value)
{
    switch (field->size) {
    case 1:  *(u8 *)at = (u8)value; break;
    case 2:  *(u16 *)at = (u16)value; break;
    default: *(u32 *)at = (u32)value; break;
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
    if (!strcmp(kind, "area") && index >= 0 && index < SCENE_AREAS) {
        *fields = area_fields;
        *field_count = COUNT(area_fields);
        *base = &areas[index];
        return true;
    }
    if (!strcmp(kind, "widget") && index >= 0 && index < SCENE_WIDGETS) {
        *fields = widget_fields;
        *field_count = COUNT(widget_fields);
        *base = &widgets[index];
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
    const field_t *fields = strstr(args, "area") ? area_fields : widget_fields;
    size_t count = strstr(args, "area") ? COUNT(area_fields)
                                        : COUNT(widget_fields);

    for (size_t f = 0; f < count; f++) {
        printf("%s:%zu ", fields[f].name, fields[f].count);
    }
    printf("\n");
    return true;
}

bool scene_command(const char *cmd, const char *args)
{
    int index = atoi(args);

    if (!strcmp(cmd, "scene_reset")) {
        reset();
        return true;
    }
    if (!strcmp(cmd, "fields")) return list_fields(args);
    if (!strcmp(cmd, "set")) return set_field(args);
    if (!strcmp(cmd, "get")) return get_fields(args);
    if (index < 0 || index >= SCENE_AREAS) return false;

    if (!strcmp(cmd, "layout")) {
        area_layout(&areas[index]);
        return true;
    }
    if (!strcmp(cmd, "popup")) {
        s32 x = -9999, y = -9999;

        area_popup_position(&areas[index], &x, &y);
        printf("%d %d\n", x, y);
        return true;
    }
    return false;
}
