/* The flat memory, the PE load, the heap, calls through addresses and the stops. */
#include "rt.h"
#include "rt_host.h"

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>

Cpu cpu;
Fpu fpu;
uint8_t *g_mem;

static uint32_t heap_top = RT_HEAP_BASE;

/* Block sizes for realloc: an open hash from address to size. */
static uint32_t *size_keys, *size_vals;
static uint32_t size_cap, size_count;

static void size_put(uint32_t addr, uint32_t size)
{
    if (size_count * 2 >= size_cap) {
        uint32_t old_cap = size_cap, *ok = size_keys, *ov = size_vals;
        size_cap = size_cap ? size_cap * 2 : 1024;
        size_keys = calloc(size_cap, 4);
        size_vals = calloc(size_cap, 4);
        size_count = 0;
        for (uint32_t i = 0; i < old_cap; i++)
            if (ok[i]) size_put(ok[i], ov[i]);
        free(ok);
        free(ov);
    }
    uint32_t i = (addr >> 4) * 2654435761u & (size_cap - 1);
    while (size_keys[i] && size_keys[i] != addr) i = (i + 1) & (size_cap - 1);
    if (!size_keys[i]) size_count++;
    size_keys[i] = addr;
    size_vals[i] = size;
}

uint32_t rt_block_size(uint32_t addr)
{
    if (!size_cap) return 0;
    uint32_t i = (addr >> 4) * 2654435761u & (size_cap - 1);
    while (size_keys[i]) {
        if (size_keys[i] == addr) return size_vals[i];
        i = (i + 1) & (size_cap - 1);
    }
    return 0;
}

void rt_set_block_size(uint32_t addr, uint32_t size) { size_put(addr, size); }

uint32_t rt_alloc(uint32_t size)
{
    uint32_t addr = heap_top;
    heap_top += ((size ? size : 1) + 15) & ~15u;
    if (heap_top > RT_HEAP_BASE + RT_HEAP_SIZE) rt_fatal("heap full");
    size_put(addr, size);
    return addr;
}

uint32_t rt_alloc_str(const char *s)
{
    uint32_t n = (uint32_t)strlen(s) + 1;
    uint32_t a = rt_alloc(n);
    memcpy(g_mem + a, s, n);
    return a;
}

uint32_t rt_heap_top(void) { return heap_top; }

static uint32_t image_base, image_size;

uint32_t rt_image_base(void) { return image_base; }
uint32_t rt_image_size(void) { return image_size; }

static uint32_t u32_at(const uint8_t *p) { uint32_t v; memcpy(&v, p, 4); return v; }
static uint16_t u16_at(const uint8_t *p) { uint16_t v; memcpy(&v, p, 2); return v; }

void rt_init(const char *exe_path)
{
    /* 4 GB and a page, so that an access at the top of the 32-bit space stays inside. */
    size_t span = (size_t)1 << 32;
    void *m = mmap(NULL, span + 0x10000, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANON | MAP_NORESERVE, -1, 0);
    if (m == MAP_FAILED) rt_fatal("cannot reserve 4 GB of address space");
    g_mem = m;

    FILE *f = fopen(exe_path, "rb");
    if (!f) rt_fatal("cannot open %s", exe_path);
    fseek(f, 0, SEEK_END);
    long n = ftell(f);
    fseek(f, 0, SEEK_SET);
    uint8_t *data = malloc((size_t)n);
    if (fread(data, 1, (size_t)n, f) != (size_t)n) rt_fatal("cannot read %s", exe_path);
    fclose(f);
    uint32_t pe = u32_at(data + 0x3c);
    uint16_t count = u16_at(data + pe + 6), optional = u16_at(data + pe + 20);
    image_base = u32_at(data + pe + 24 + 28);
    image_size = u32_at(data + pe + 24 + 56);
    for (uint16_t i = 0; i < count; i++) {
        const uint8_t *s = data + pe + 24 + optional + i * 40;
        uint32_t vsize = u32_at(s + 8), va = u32_at(s + 12), rsize = u32_at(s + 16), rptr = u32_at(s + 20);
        uint32_t len = vsize && vsize < rsize ? vsize : rsize;
        memcpy(g_mem + image_base + va, data + rptr, len);
    }
    free(data);
    for (uint32_t i = 0; i < rt_iat_size; i++) wr32(g_mem, rt_iat[i].iat, rt_iat[i].stub);
    fpu_init();
    fpu.cw = 0x27f;                        /* what Windows gives a new process */
}

void fpu_init(void)
{
    memset(&fpu, 0, sizeof fpu);
    fpu.cw = 0x37f;
}

/* 80-bit extended to double and back. */
double rdf80(const uint8_t *m, uint32_t a)
{
    uint64_t mant;
    uint16_t se;
    memcpy(&mant, m + a, 8);
    memcpy(&se, m + a + 8, 2);
    int sign = se >> 15, exp = se & 0x7fff;
    double v;
    if (exp == 0 && mant == 0) v = 0.0;
    else if (exp == 0x7fff) v = (mant << 1) ? NAN : INFINITY;
    else v = ldexp((double)mant, exp - 16383 - 63);
    return sign ? -v : v;
}

void wrf80(uint8_t *m, uint32_t a, double v)
{
    uint64_t mant = 0;
    uint16_t se = signbit(v) ? 0x8000 : 0;
    double x = fabs(v);
    if (x == 0.0) {
    } else if (isinf(x)) {
        se |= 0x7fff; mant = 1ull << 63;
    } else if (isnan(x)) {
        se |= 0x7fff; mant = 3ull << 62;
    } else {
        int e;
        double fr = frexp(x, &e);                 /* x = fr * 2^e, 0.5 <= fr < 1 */
        mant = (uint64_t)ldexp(fr, 64);
        se |= (uint16_t)(e - 1 + 16383);
    }
    memcpy(m + a, &mant, 8);
    memcpy(m + a + 8, &se, 2);
}

/* Calls through an address. */
RtFn rt_lookup(uint32_t target)
{
    uint32_t lo = 0, hi = rt_table_size;
    while (lo < hi) {
        uint32_t mid = (lo + hi) / 2;
        if (rt_table[mid].addr < target) lo = mid + 1;
        else hi = mid;
    }
    return lo < rt_table_size && rt_table[lo].addr == target ? rt_table[lo].fn : NULL;
}

void rt_call(uint32_t target)
{
    RtFn fn = rt_lookup(target);
    if (!fn) rt_fatal("call to %#x, which is not a recompiled function (from %#x)", target, rd32(g_mem, cpu.esp));
    fn();
}

void rt_fatal(const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    fprintf(stderr, "recomp: ");
    vfprintf(stderr, fmt, ap);
    fprintf(stderr, "\n");
    va_end(ap);
    exit(70);
}

void rt_unimpl(uint32_t at, const char *text) { rt_fatal("instruction at %#x is not translated: %s", at, text); }
void rt_fell_off(uint32_t at) { rt_fatal("execution reached %#x, where the recompiler found no code", at); }
void rt_bad_jump(uint32_t at, uint32_t target) { rt_fatal("jump table at %#x: target %#x is not in the table", at, target); }
void rt_missing_import(const char *name) { rt_fatal("import %s has no replacement (called from %#x)", name, rd32(g_mem, cpu.esp)); }
void rt_divide_error(uint32_t at) { rt_fatal("divide error at %#x", at); }
