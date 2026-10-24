/* The C library's qsort of the original, FUN_00945333 (cdecl: base, num, width, comp), in C.
 *
 * Why: the profile of a new game in the browser (2026-10-24) put 29% of the time in the recompiled qsort
 * itself and another 28% in rt_call, which most of the comparator calls of the sorts go through. Each
 * comparison in the generated code pushes the arguments through the flat memory, looks the comparator up
 * again by address and keeps the lazy flags of the loop. Here the comparator is looked up once per sort.
 *
 * This is the same algorithm as the original, step by step (the generated code is in fn_075.c, from
 * 0x945333; short sort 0x945487; swap 0x9454d5), because qsort is not stable and the order of equal
 * elements decides what the game does later (rule Q152):
 *   - fewer than 2 elements or a width of 0: nothing.
 *   - a range of 8 elements or fewer: short sort. It takes the largest element (the first one that is
 *     larger than the current largest, comp(p, max) > 0), swaps it with the last one, and shortens the range.
 *   - else: swap the middle (lo + size / 2 * width) with the first, then partition with loguy going up
 *     while comp(loguy, lo) <= 0 and higuy going down while comp(higuy, lo) >= 0, swapping the two where
 *     they stop, then swap lo with higuy. The smaller side goes on the stack and the larger side
 *     is sorted next (a stack that is at most log2 of the count deep).
 * The comparator is called as comp(a, b) with the arguments pushed b first (cdecl), as in the original.
 * The swap of two elements is by bytes and does nothing when both are the same element. */
#include "win.h"

#define CUTOFF 8
#define STKSIZ 64                     /* the original's is 30: the depth is at most log2 of the count */

typedef struct {
    RtFn fn;
    uint32_t target;                  /* the comparator's address (rt_call_target, for host functions) */
    uint32_t esp0;                    /* esp at the entry: for the unwinding check of RT_AFTER_CALL */
} Cmp;

static void swap_bytes(uint32_t a, uint32_t b, uint32_t width)
{
    if (a == b) return;
    uint8_t *p = g_mem + a, *q = g_mem + b;
    while (width--) {
        uint8_t t = *p;
        *p++ = *q;
        *q++ = t;
    }
}

/* comp(a, b). *gone is set when the call unwound past this function (longjmp): then nothing more runs. */
static inline int32_t compare(const Cmp *c, uint32_t a, uint32_t b, int *gone)
{
    cpu.esp -= 4;
    wr32(g_mem, cpu.esp, b);
    cpu.esp -= 4;
    wr32(g_mem, cpu.esp, a);
    cpu.esp -= 4;
    wr32(g_mem, cpu.esp, RT_RETURN_ADDR);
    rt_call_target = c->target;
    c->fn();
    if (__builtin_expect(rt_unwinding, 0) && rt_unwound(c->esp0)) {
        *gone = 1;
        return 0;
    }
    cpu.esp += 8;                     /* cdecl: the caller removes the two arguments */
    return (int32_t)cpu.eax;
}

/* 1 when the call unwound. */
static int short_sort(const Cmp *c, uint32_t lo, uint32_t hi, uint32_t width)
{
    int gone = 0;
    while (hi > lo) {
        uint32_t max = lo;
        for (uint32_t p = lo + width; p <= hi; p += width) {
            int32_t r = compare(c, p, max, &gone);
            if (gone) return 1;
            if (r > 0) max = p;
        }
        swap_bytes(max, hi, width);
        hi -= width;
    }
    return 0;
}

RT_REPLACE(00945333, crt_qsort)
{
    uint32_t base = rt_arg(0), num = rt_arg(1), width = rt_arg(2), comp = rt_arg(3);
    if (num < 2 || width == 0) { rt_ret_cdecl(cpu.eax); return; }

    Cmp c = { rt_lookup(comp), comp, cpu.esp };
    if (!c.fn) rt_fatal("qsort: the comparison %#x is not a recompiled function", comp);

    uint32_t lostk[STKSIZ], histk[STKSIZ];
    int stkptr = 0, gone = 0;
    uint32_t lo = base, hi = base + width * (num - 1);

    for (;;) {
        uint32_t size = (hi - lo) / width + 1;
        if (size <= CUTOFF) {
            if (short_sort(&c, lo, hi, width)) return;
        } else {
            uint32_t mid = lo + (size / 2) * width;
            swap_bytes(mid, lo, width);
            uint32_t loguy = lo, higuy = hi + width;
            for (;;) {
                for (;;) {
                    loguy += width;
                    if (loguy > hi) break;
                    int32_t r = compare(&c, loguy, lo, &gone);
                    if (gone) return;
                    if (r > 0) break;
                }
                for (;;) {
                    higuy -= width;
                    if (higuy <= lo) break;
                    int32_t r = compare(&c, higuy, lo, &gone);
                    if (gone) return;
                    if (r < 0) break;
                }
                if (higuy < loguy) break;
                swap_bytes(loguy, higuy, width);
            }
            swap_bytes(lo, higuy, width);
            if ((int32_t)(higuy - 1 - lo) >= (int32_t)(hi - loguy)) {
                if (lo + width < higuy) { lostk[stkptr] = lo; histk[stkptr] = higuy - width; stkptr++; }
                if (loguy < hi) { lo = loguy; continue; }
            } else {
                if (loguy < hi) { lostk[stkptr] = loguy; histk[stkptr] = hi; stkptr++; }
                if (lo + width < higuy) { hi = higuy - width; continue; }
            }
        }
        if (--stkptr < 0) break;
        lo = lostk[stkptr];
        hi = histk[stkptr];
    }
    rt_ret_cdecl(cpu.eax);
}
