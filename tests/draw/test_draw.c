/* Test of the runtime draw code for the Team views screen (draw.c): lines, filled polygons and the
 * pitch, on a buffer in host memory. Build and run draw_test, or ctest -R draw. */
#include "draw.h"

#include <stdio.h>
#include <string.h>

static int failures;
#define CHECK(c) do { if (!(c)) { fprintf(stderr, "%s:%d: CHECK(%s) failed\n", __FILE__, __LINE__, #c); failures++; } } while (0)

#define W 64
#define H 48
static uint16_t buffer[H][W];
static DrawTarget target = {(uint8_t *)buffer, W * 2, W, H};

static void clear(void) { memset(buffer, 0, sizeof buffer); }

static int count(uint16_t colour)
{
    int n = 0;
    for (int y = 0; y < H; y++)
        for (int x = 0; x < W; x++) n += buffer[y][x] == colour;
    return n;
}

static void lines(void)
{
    clear();
    draw_line(&target, 2, 3, 12, 3, 0xffff);
    CHECK(count(0xffff) == 11);
    CHECK(buffer[3][2] == 0xffff && buffer[3][12] == 0xffff);

    clear();
    draw_line(&target, 5, 20, 5, 10, 0xffff);      /* upwards */
    CHECK(count(0xffff) == 11);
    CHECK(buffer[10][5] == 0xffff && buffer[20][5] == 0xffff);

    clear();
    draw_line(&target, 0, 0, 20, 10, 0x1234);      /* a diagonal: one pixel per column */
    CHECK(count(0x1234) == 21);
    CHECK(buffer[0][0] == 0x1234 && buffer[10][20] == 0x1234);
    for (int x = 0; x <= 20; x++) {
        int n = 0;
        for (int y = 0; y < H; y++) n += buffer[y][x] == 0x1234;
        CHECK(n == 1);
    }

    clear();
    draw_line(&target, -10, 5, 100, 5, 0xffff);    /* clipped to the buffer */
    CHECK(count(0xffff) == W);
}

static void polygons(void)
{
    clear();
    int square[] = {10, 10, 20, 10, 20, 20, 10, 20};
    draw_polygon(&target, square, 4, 0x07e0, 256);
    CHECK(count(0x07e0) == 100);                   /* pixel centres inside: 10..19 by 10..19 */
    CHECK(buffer[10][10] == 0x07e0 && buffer[19][19] == 0x07e0 && buffer[20][20] == 0);

    clear();
    int triangle[] = {0, 0, 40, 0, 0, 40};
    draw_polygon(&target, triangle, 3, 0xf800, 256);
    int n = count(0xf800);
    CHECK(n >= 780 && n <= 820);                   /* area 800 */
    CHECK(buffer[39][39] == 0 && buffer[1][1] == 0xf800);

    clear();
    int concave[] = {0, 0, 30, 0, 30, 30, 15, 10, 0, 30};   /* a notch from the bottom */
    draw_polygon(&target, concave, 5, 0x001f, 256);
    CHECK(buffer[25][15] == 0);                    /* inside the notch */
    CHECK(buffer[25][2] == 0x001f && buffer[25][28] == 0x001f);

    clear();
    int off[] = {-20, -20, 100, -20, 100, 100, -20, 100};   /* bigger than the buffer */
    draw_polygon(&target, off, 4, 0xffff, 256);
    CHECK(count(0xffff) == W * H);

    /* Half alpha over black: each channel halves. */
    clear();
    draw_polygon(&target, square, 4, 0xffff, 128);
    CHECK(buffer[15][15] == ((15 << 11) | (31 << 5) | 15));
}

static void pitch(void)
{
    clear();
    draw_pitch(&target, 4, 2, 59, 45);
    CHECK(buffer[0][0] == 0 && buffer[47][63] == 0 && buffer[1][4] == 0 && buffer[2][60] == 0);
    CHECK(buffer[2][4] == DRAW_PITCH_LINE && buffer[45][59] == DRAW_PITCH_LINE);   /* corners */
    CHECK(buffer[23][31] == DRAW_PITCH_LINE || buffer[24][31] == DRAW_PITCH_LINE); /* halfway line */
    int grass = 0;
    for (int y = 3; y < 45; y++)
        for (int x = 5; x < 59; x++) grass += buffer[y][x] == DRAW_PITCH_GRASS || buffer[y][x] == DRAW_PITCH_STRIPE;
    CHECK(grass > 54 * 42 * 3 / 4);
    CHECK(count(DRAW_PITCH_STRIPE) > 0);
}

int main(void)
{
    lines();
    polygons();
    pitch();
    if (failures) {
        fprintf(stderr, "draw: %d checks failed\n", failures);
        return 1;
    }
    printf("draw: all checks passed\n");
    return 0;
}
