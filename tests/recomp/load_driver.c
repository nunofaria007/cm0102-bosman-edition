/* Runs the recompiled database load, FUN_00528540(data folder, 1, 0), and writes the memory that
 * tests/recomp/test_load.py compares with the Unicorn run.
 *
 * Usage: load_driver <cm0102.exe> <data folder> <output folder>
 * Writes image.bin, heap.bin, stack.bin and page0.bin to the output folder and prints
 * "result <eax> heap_top <addr> seconds <s>". */
#include "rt.h"
#include "rt_host.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

void f_00528540(void);
extern const char *rt_data_dir;      /* load_rt.c */

static void dump(const char *dir, const char *name, uint32_t start, uint32_t end)
{
    char path[1024];
    snprintf(path, sizeof path, "%s/%s", dir, name);
    FILE *f = fopen(path, "wb");
    if (!f || fwrite(g_mem + start, 1, end - start, f) != end - start) rt_fatal("cannot write %s", path);
    fclose(f);
}

static void push(uint32_t v)
{
    cpu.esp -= 4;
    wr32(g_mem, cpu.esp, v);
}

int main(int argc, char **argv)
{
    if (argc != 4) {
        fprintf(stderr, "usage: %s <cm0102.exe> <data folder> <output folder>\n", argv[0]);
        return 2;
    }
    static char data[1024];
    snprintf(data, sizeof data, "%s", argv[2]);
    size_t n = strlen(data);
    while (n > 1 && data[n - 1] == '/') data[--n] = 0;
    rt_data_dir = data;
    rt_heap_bump = 1;
    rt_init(argv[1]);

    /* The state before the load, as in tests/recomp/reference.py run_load(). */
    uint32_t settings = rt_alloc(0x400);
    wr32(g_mem, 0xae3138, settings);
    wr32(g_mem, 0x9eb2dc, 0);
    char folder[1100];
    snprintf(folder, sizeof folder, "%s/", data);
    uint32_t path = rt_alloc_str(folder);

    cpu.esp = RT_STACK_TOP;
    push(0);
    push(1);
    push(path);
    push(RT_RETURN_ADDR);
    struct timespec t0, t1;
    clock_gettime(CLOCK_MONOTONIC, &t0);
    f_00528540();
    clock_gettime(CLOCK_MONOTONIC, &t1);
    double seconds = (double)(t1.tv_sec - t0.tv_sec) + (double)(t1.tv_nsec - t0.tv_nsec) / 1e9;

    uint32_t base = rt_image_base();
    dump(argv[3], "image.bin", base, base + rt_image_size());
    dump(argv[3], "heap.bin", RT_HEAP_BASE, rt_heap_top());
    dump(argv[3], "stack.bin", RT_STACK_BASE, RT_STACK_TOP);
    dump(argv[3], "page0.bin", 0, 0x1000);
    printf("result %u heap_top %#x seconds %.3f\n", cpu.eax, rt_heap_top(), seconds);
    fprintf(stderr, "calls:");
    rt_print_counts();
    return 0;
}
