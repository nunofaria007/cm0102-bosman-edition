/* Block counts of the functions that were generated with --count-blocks (CM0102_COUNT_BLOCKS in CMake).
 *
 * A profile of the browser build names the functions that take the time, not the part of a big function.
 * With the entries of such functions in CM0102_COUNT_BLOCKS, every labelled block of them counts its runs
 * (tools/recomp/translate.py, rt_bb in table.c). The counts say how often each part of a function runs: the
 * blocks of the loop of fill_shortlist_slot (0x83f3a0) that run per candidate, and how many candidates stop
 * at each test. Without the option the arrays hold one unused slot and nothing counts.
 *
 * In the browser console (the build exports both):
 *   Module._rt_bb_reset()        before the part to measure (for example before "Start a new game")
 *   Module._rt_bb_report(150)    afterwards: the 150 most run blocks, and for each counted function how many
 *                                times it was entered
 * The report goes to stdout, so it shows in the console. On the other builds the same two functions can be
 * called from a debugger. */
#include "rt.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#define BB_EXPORT EMSCRIPTEN_KEEPALIVE
#else
#define BB_EXPORT
#endif

extern const uint32_t rt_bb_count;
extern const uint32_t rt_bb_addr[];
extern const uint32_t rt_bb_entry[];

BB_EXPORT void rt_bb_reset(void)
{
    memset(rt_bb, 0, (rt_bb_count ? rt_bb_count : 1) * sizeof rt_bb[0]);
}

static int by_count(const void *a, const void *b)
{
    uint64_t x = rt_bb[*(const uint32_t *)a], y = rt_bb[*(const uint32_t *)b];
    return x < y ? 1 : x > y ? -1 : 0;
}

/* Prints the counted functions with their number of entries, then the top blocks by runs. */
BB_EXPORT void rt_bb_report(int top)
{
    if (!rt_bb_count) {
        printf("rt_bb_report: no function was generated with --count-blocks (CM0102_COUNT_BLOCKS)\n");
        return;
    }
    printf("rt_bb_report: %u counted blocks\n", rt_bb_count);
    for (uint32_t i = 0; i < rt_bb_count; i++)
        if (rt_bb_addr[i] == rt_bb_entry[i])
            printf("function %#x: entered %llu times\n", rt_bb_entry[i], (unsigned long long)rt_bb[i]);
    uint32_t *order = malloc(rt_bb_count * sizeof *order);
    if (!order) return;
    for (uint32_t i = 0; i < rt_bb_count; i++) order[i] = i;
    qsort(order, rt_bb_count, sizeof *order, by_count);
    printf("block (function: address runs)\n");
    for (uint32_t k = 0; k < rt_bb_count && (int)k < top; k++) {
        uint32_t i = order[k];
        if (!rt_bb[i]) break;
        printf("%#x: %#x %llu\n", rt_bb_entry[i], rt_bb_addr[i], (unsigned long long)rt_bb[i]);
    }
    free(order);
}
