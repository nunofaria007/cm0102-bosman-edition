/*
 * Runs functions of the port for the differential test (tests/diff/run.py).
 * Reads one text command per line on stdin and writes the result to stdout.
 */
#include "display.h"
#include "gui_utils.h"
#include "port_scene.h"
#include "port_text.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define FB_PIXELS (CM_INTERNAL_WIDTH * CM_INTERNAL_HEIGHT)
#define MAX_GLYPH (2 + 255 * 255)

static void fill_noise(u32 seed)
{
    for (s32 i = 0; i < FB_PIXELS; i++) {
        seed = seed * 1664525u + 1013904223u;
        g_display.framebuffer[i] = (u16)(seed >> 16);
    }
}

void print_image(const cm_image_t *img)
{
    if (!img) {
        printf("0\n");
        return;
    }
    printf("%d %d %d %d %d %d %d %u %u %u %d\n",
           img->width, img->height, img->data_size,
           img->bits_per_pixel, img->bytes_per_pixel, img->field_18,
           img->significant_bits, img->r_mask, img->g_mask, img->b_mask,
           img->field_2c);
    fwrite(img->pixels, 1, (size_t)img->data_size, stdout);
}

/* "glyph x y colour hex": the hex digits are a whole glyph table, or "null". */
static bool run_glyph(const char *args)
{
    static u8 glyph[MAX_GLYPH];
    static char hex[2 * MAX_GLYPH + 1];
    int x, y, colour;

    if (sscanf(args, "%d %d %d %130054s", &x, &y, &colour, hex) != 4) {
        return false;
    }
    if (!strcmp(hex, "null")) {
        gui_draw_glyph(x, y, (u16)colour, NULL);
        return true;
    }

    size_t count = strlen(hex) / 2;
    for (size_t i = 0; i < count; i++) {
        unsigned value;
        if (sscanf(hex + 2 * i, "%2x", &value) != 1) return false;
        glyph[i] = (u8)value;
    }
    if (strlen(hex) % 2 || count < 2 || count != 2u + glyph[0] * glyph[1]) {
        return false;
    }
    gui_draw_glyph(x, y, (u16)colour, glyph);
    return true;
}

int main(void)
{
    static char line[2 * MAX_GLYPH + 256];

    g_display.framebuffer = calloc(FB_PIXELS, sizeof(u16));
    if (!g_display.framebuffer) return 1;

    while (fgets(line, sizeof(line), stdin)) {
        char cmd[16];
        char path[1024] = "";
        int used = 0;
        int a[8] = {0};

        if (sscanf(line, "%15s%n", cmd, &used) != 1) continue;
        const char *args = line + used;
        if (strcmp(cmd, "glyph")) {
            sscanf(args, "%d %d %d %d %d %d %d %d",
                   &a[0], &a[1], &a[2], &a[3], &a[4], &a[5], &a[6], &a[7]);
        }

        if (!strcmp(cmd, "fill")) {
            fill_noise((u32)a[0]);
        } else if (!strcmp(cmd, "dump")) {
            fwrite(g_display.framebuffer, sizeof(u16), FB_PIXELS, stdout);
        } else if (!strcmp(cmd, "clip")) {
            gui_rect_t r = {0};
            bool ok = gui_clip(a[0], a[1], a[2], a[3], &r);
            printf("%d %d %d %d %d\n", ok, r.x1, r.y1, r.x2, r.y2);
        } else if (!strcmp(cmd, "clip_null")) {
            printf("%d\n", gui_clip(a[0], a[1], a[2], a[3], NULL));
        } else if (!strcmp(cmd, "scale")) {
            printf("%u\n", gui_scale_colour((u16)a[0], (u8)a[1]));
        } else if (!strcmp(cmd, "line")) {
            gui_line(a[0], a[1], a[2], a[3], (u32)a[4], (u16)a[5]);
        } else if (!strcmp(cmd, "rect")) {
            gui_rect(a[0], a[1], a[2], a[3], (u32)a[4], (u16)a[5]);
        } else if (!strcmp(cmd, "palette")) {
            printf("%u %u %u %u %u %u\n", GUI_COLOUR_NAVY, GUI_COLOUR_YELLOW,
                   GUI_COLOUR_BLACK, GUI_COLOUR_LIGHT_GREY, GUI_COLOUR_ORANGE,
                   GUI_COLOUR_RED);
        } else if (!strcmp(cmd, "panel")) {
            gui_panel(a[0], a[1], a[2], a[3], (u32)a[4], (u16)a[5]);
        } else if (!strcmp(cmd, "text_calls")) {
            text_print_calls();
        } else if (!strcmp(cmd, "darken")) {
            gui_darken(a[0], a[1], a[2], a[3]);
        } else if (!strcmp(cmd, "glyph")) {
            if (!run_glyph(args)) {
                fprintf(stderr, "port_driver: bad glyph command\n");
                return 1;
            }
        } else if (!strcmp(cmd, "load")) {
            sscanf(args, "%1023s", path);
            cm_image_t *img = rgn_load(path);
            print_image(img);
            rgn_free(img);
        } else if (!strcmp(cmd, "blit")) {
            sscanf(args, "%*d %*d %1023s", path);
            cm_image_t *img = rgn_load(path);
            gui_blit(a[0], a[1], img);
            rgn_free(img);
        } else if (!strcmp(cmd, "capture")) {
            cm_image_t *img = gui_capture(a[0], a[1], a[2], a[3], NULL);
            print_image(img);
            rgn_free(img);
        } else if (!strcmp(cmd, "capture2")) {
            cm_image_t *first = gui_capture(a[0], a[1], a[2], a[3], NULL);
            cm_image_t *second = gui_capture(a[4], a[5], a[6], a[7], first);
            printf("%d ", second == first);
            print_image(second);
            if (second != first) rgn_free(second);
            rgn_free(first);
        } else if (!scene_command(cmd, args)) {
            fprintf(stderr, "port_driver: bad command: %s", line);
            return 1;
        }
        fflush(stdout);
    }
    return 0;
}
