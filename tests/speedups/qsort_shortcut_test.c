#include <stdarg.h>
#include "win.h"
#include <stdlib.h>
#include <time.h>
Cpu cpu; uint8_t *g_mem; int rt_unwinding; uint32_t rt_call_target;
Fpu fpu;
void f_007c6d40(void);
RtFn rt_lookup(uint32_t t) { return t == 0x7c6d40u || t == 0x7c6d41u ? f_007c6d40 : 0; }
void rt_fatal(const char *f, ...) { va_list ap; va_start(ap, f); vfprintf(stderr, f, ap); va_end(ap); fputc('\n', stderr); exit(1); }
int32_t cmp_language_test(uint32_t, uint32_t);
void f_00945333(void); void f_00946464(void);
static void push_call(RtFn fn, int n, uint32_t *args) {
    cpu.esp = 0x80000; for (int i = n - 1; i >= 0; i--) { cpu.esp -= 4; wr32(g_mem, cpu.esp, args[i]); }
    cpu.esp -= 4; wr32(g_mem, cpu.esp, RT_RETURN_ADDR); fn();
}
void rt_call(uint32_t target) { rt_lookup(target)(); }
void f_gen_bsearch(void);
int main(void) {
    g_mem = calloc(1, 1 << 20); srand(7);
    /* 1. the native comparison against the generated function, on random and edge pairs */
    long bad = 0, n = 0;
    uint32_t ids[] = {0, 1, 2, 0x7fffffffu, 0x80000000u, 0xffffffffu, 100, 101};
    int8_t lg[] = {-1, 0, 1, 5, 127, -128, -2};
    for (int i = 0; i < 400000; i++) {
        uint32_t a = 0x1000, b = 0x1010;
        uint32_t ia = i % 3 ? ids[rand() % 8] : (uint32_t)rand(), ib = i % 2 ? ia : ids[rand() % 8];
        wr32(g_mem, a, ia); wr32(g_mem, b, ib);
        g_mem[a + 4] = (uint8_t)lg[rand() % 7]; g_mem[b + 4] = (uint8_t)lg[rand() % 7];
        uint32_t aa = (i % 50 == 0) ? 0 : a, bb = (i % 51 == 0) ? 0 : b;
        uint32_t args[2] = { aa, bb };
        push_call(f_007c6d40, 2, args);
        int32_t want = (int32_t)cpu.eax;
        int32_t got = cmp_language_test(aa, bb);
        n++; if (got != want) { if (bad++ < 5) printf("MISMATCH %u/%d vs %u/%d: got %d want %d\n", ia, (int8_t)g_mem[a+4], ib, (int8_t)g_mem[b+4], got, want); }
    }
    printf("comparator: %ld pairs, %ld mismatches\n", n, bad);
    /* 2. qsort and bsearch with native and generated comparison: the arrays must be the same, byte for byte */
    long qbad = 0, sbad = 0;
    for (int round = 0; round < 300; round++) {
        uint32_t cnt = (uint32_t)(rand() % 400) + (round % 7 == 0 ? 0 : 2), base = 0x2000;
        for (uint32_t i = 0; i < cnt; i++) {
            wr32(g_mem, base + 6 * i, (uint32_t)(rand() % 40)); g_mem[base + 6 * i + 4] = (uint8_t)lg[rand() % 7]; g_mem[base + 6 * i + 5] = (uint8_t)(rand() % 11);
        }
        uint8_t *copy = malloc(cnt * 6 + 1); memcpy(copy, g_mem + base, cnt * 6);
        uint32_t args[4] = { base, cnt, 6, 0x7c6d40u };
        push_call(f_00945333, 4, args);                      /* native path (comp == CMP_LANGUAGE) */
        uint8_t *a = malloc(cnt * 6 + 1); memcpy(a, g_mem + base, cnt * 6);
        memcpy(g_mem + base, copy, cnt * 6);
        { uint32_t ga[4] = { base, cnt, 6, 0x7c6d41u }; push_call(f_00945333, 4, ga); }   /* other address: generated comparator */
        if (memcmp(a, g_mem + base, cnt * 6)) qbad++;
        for (int k = 0; k < 60; k++) {
            uint32_t key = 0x9000; wr32(g_mem, key, (uint32_t)(rand() % 45)); g_mem[key + 4] = (uint8_t)lg[rand() % 7];
            uint32_t ba[5] = { key, base, cnt, 6, 0x7c6d40u };
            push_call(f_00946464, 5, ba); uint32_t r1 = cpu.eax;
            uint32_t gb[5] = { key, base, cnt, 6, 0x7c6d41u }; push_call(f_00946464, 5, gb); uint32_t r2 = cpu.eax;
            uint32_t gc[5] = { key, base, cnt, 6, 0x7c6d40u }; push_call(f_gen_bsearch, 5, gc); uint32_t r3 = cpu.eax;
            if (r1 != r2 || r1 != r3) sbad++;
        }
        free(copy); free(a);
    }
    printf("qsort: %ld of 300 differ; bsearch: %ld of 18000 differ\n", qbad, sbad);
    /* 3. the shortcut for a sorted table plus one entry: the same bytes as the sort of the original */
    long fbad = 0, fused = 0;
    for (int round = 0; round < 6000; round++) {
        uint32_t cnt = (uint32_t)(rand() % 300) + 9, base = 0x2000;
        int kind = round % 8;
        uint32_t id = 0;
        for (uint32_t i = 0; i < cnt; i++) {
            int step = (kind == 3) ? rand() % 2 : 1 + rand() % 4;     /* kind 3 has equal ids */
            id += (uint32_t)step;
            wr32(g_mem, base + 6 * i, kind == 4 && i == cnt / 2 ? 0x80000005u : id);
            g_mem[base + 6 * i + 4] = (uint8_t)(kind == 3 ? (i % 5) : rand() % 6);
            if (kind == 5 && i == 3) g_mem[base + 6 * i + 4] = 0xff;
            g_mem[base + 6 * i + 5] = (uint8_t)(rand() % 11);
        }
        /* sort the first cnt-1 entries (strictly, by the comparator) except in kind 6 and 7 */
        { uint32_t ga[4] = { base, cnt - 1, 6, 0x7c6d41u }; if (kind != 6) push_call(f_00945333, 4, ga); }
        uint32_t last = base + 6 * (cnt - 1);
        uint32_t pick = rand() % cnt;                                /* the new entry: copy or new key */
        if (kind == 7) { wr32(g_mem, last, rand() % (id + 3)); g_mem[last + 4] = (uint8_t)(rand() % 6); }
        else if (rand() % 3 == 0) { memcpy(g_mem + last, g_mem + base + 6 * (pick % (cnt - 1)), 5); g_mem[last + 5] = 99; }
        else { wr32(g_mem, last, (uint32_t)(rand() % (id + 3))); g_mem[last + 4] = (uint8_t)(rand() % 6); g_mem[last + 5] = 7; }
        if (rand() % 20 == 0) g_mem[last + 4] = 0xff;
        uint8_t *copy = malloc(cnt * 6); memcpy(copy, g_mem + base, cnt * 6);
        uint32_t na[4] = { base, cnt, 6, 0x7c6d40u }; push_call(f_00945333, 4, na);
        uint8_t *a = malloc(cnt * 6); memcpy(a, g_mem + base, cnt * 6);
        memcpy(g_mem + base, copy, cnt * 6);
        uint32_t oa[4] = { base, cnt, 6, 0x7c6d41u }; push_call(f_00945333, 4, oa);
        if (memcmp(a, g_mem + base, cnt * 6)) fbad++;
        free(copy); free(a);
    }
    printf("shortcut: %ld of 6000 differ\n", fbad); (void)fused;
    return bad || qbad || sbad || fbad;
}