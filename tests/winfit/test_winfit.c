/* The size of a new window on a small screen (winfit.h, 0.4.0 slice 10). */
#include "winfit.h"

#include <stdio.h>

static int failed;
#define CHECK(c) do { if (!(c)) { printf("FAIL line %d: %s\n", __LINE__, #c); failed = 1; } } while (0)

int main(void)
{
    int w, h;
    /* It fits: the size does not change. */
    winfit_size(1280, 800, 1920, 1050, &w, &h);
    CHECK(w == 1280 && h == 800);
    winfit_size(1920, 1080, 3840, 2100, &w, &h);
    CHECK(w == 1920 && h == 1080);
    /* HD on a 1080p screen with a menu bar and a title bar: the height limits, the shape stays 16:9. */
    winfit_size(1920, 1080, 1920, 1020, &w, &h);
    CHECK(h == 1020 && w == 1813);
    /* A narrow screen: the width limits. */
    winfit_size(1920, 1080, 1366, 740, &w, &h);
    CHECK(w == 1315 && h == 740);
    winfit_size(1920, 1080, 1280, 1000, &w, &h);
    CHECK(w == 1280 && h == 720);
    /* No usable size known: the size does not change. */
    winfit_size(1920, 1080, 0, 0, &w, &h);
    CHECK(w == 1920 && h == 1080);
    puts(failed ? "FAIL" : "PASS");
    return failed;
}
