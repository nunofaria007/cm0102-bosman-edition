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

/* The comparators that run natively. Profile of a new game in the browser (2026-10-29): 12% of the time in
 * the comparator of the language entries alone (0x7c6d40, PlayerStats), a function of about twenty
 * instructions that is called hundreds of millions of times by qsort here and by bsearch below. A call
 * through the flat memory costs far more than the comparison, so these are done in C on the flat memory, in
 * the same order of tests as the generated code. With CM0102_QSORT_VERIFY the generated comparator also runs
 * and the two results must agree. */
#define CMP_LANGUAGE 0x007c6d40u      /* player_stats_compare_language_entries(a, b): 6-byte entries */

/* Staff id (u32 at +0) and then language (i8 at +4), where language -1 is a wildcard that equals any. A NULL
 * pointer sorts first. The difference is of 32 bits and wraps, as in the original. */
static inline int32_t cmp_language(uint32_t a, uint32_t b)
{
    if (a == 0) return -1;
    if (b == 0) return 1;
    uint32_t ia = rd32(g_mem, a), ib = rd32(g_mem, b);
    if (ia != ib) return (int32_t)(ia - ib);
    uint8_t la = g_mem[a + 4], lb = g_mem[b + 4];
    if (la == 0xff || lb == 0xff) return 0;
    return (int32_t)(int8_t)la - (int32_t)(int8_t)lb;
}

#define CUTOFF 8
#define STKSIZ 64                     /* the original's is 30: the depth is at most log2 of the count */

typedef struct {
    RtFn fn;
    uint32_t target;                  /* the comparator's address (rt_call_target, for host functions) */
    uint32_t esp0;                    /* esp at the entry: for the unwinding check of RT_AFTER_CALL */
    int native;                       /* 1 when the comparison is done by cmp_language (CMP_LANGUAGE) */
} Cmp;

static void swap_bytes(uint32_t a, uint32_t b, uint32_t width)
{
    if (a == b) return;
    uint8_t *p = g_mem + a, *q = g_mem + b;
    for (; width >= 4; width -= 4, p += 4, q += 4) {       /* the same exchange, four bytes at a time */
        uint32_t t, u;
        memcpy(&t, p, 4); memcpy(&u, q, 4);
        memcpy(p, &u, 4); memcpy(q, &t, 4);
    }
    while (width--) {
        uint8_t t = *p;
        *p++ = *q;
        *q++ = t;
    }
}

/* comp(a, b). *gone is set when the call unwound past this function (longjmp): then nothing more runs. */
static inline int32_t compare_generated(const Cmp *c, uint32_t a, uint32_t b, int *gone)
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

static inline __attribute__((always_inline)) int32_t compare(const Cmp *c, uint32_t a, uint32_t b, int *gone)
{
    if (c->native) {
#ifdef CM0102_QSORT_VERIFY
        int32_t want = compare_generated(c, a, b, gone);
        if (*gone) return 0;
        int32_t got = cmp_language(a, b);
        if (got != want) rt_fatal("qsort/bsearch: native language comparison %d, generated %d (%#x, %#x)", got, want, a, b);
        return want;
#else
        return cmp_language(a, b);
#endif
    }
    return compare_generated(c, a, b, gone);
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

    Cmp c = { rt_lookup(comp), comp, cpu.esp, comp == CMP_LANGUAGE };
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

/* The C library's bsearch of the original, FUN_00946464 (cdecl: key, base, num, width, comp), in C, with the
 * same steps (the generated code is in fn_*.c from 0x946464): the middle is element half of an odd count and
 * half - 1 of an even one; comp(key, middle) = 0 finds it, above 0 goes up, below 0 goes down; one element
 * left is compared once. The test of a language entry is the main caller (PlayerStats::get_language_fluency,
 * 0x7c6830), so this is native for that comparator too. Returns the address or 0. */
RT_REPLACE(00946464, crt_bsearch)
{
    uint32_t key = rt_arg(0), base = rt_arg(1), num = rt_arg(2), width = rt_arg(3), comp = rt_arg(4);
    Cmp c = { rt_lookup(comp), comp, cpu.esp, comp == CMP_LANGUAGE };
    if (!c.fn) rt_fatal("bsearch: the comparison %#x is not a recompiled function", comp);

    uint32_t hi = base + width * (num - 1), found = 0;
    int gone = 0;
    if (base <= hi) {
        for (;;) {
            uint32_t half = num >> 1;
            if (half == 0) {
                if (num != 0) {
                    int32_t r = compare(&c, key, base, &gone);
                    if (gone) return;
                    if (r == 0) found = base;
                }
                break;
            }
            uint32_t odd = num & 1;
            uint32_t mid = base + (odd ? half : half - 1) * width;
            int32_t r = compare(&c, key, mid, &gone);
            if (gone) return;
            if (r == 0) { found = mid; break; }
            if (r > 0) { base = mid + width; num = half; }
            else { hi = mid - width; num = odd ? half : half - 1; }
            if (base > hi) break;
        }
    }
    rt_ret_cdecl(found);
}
