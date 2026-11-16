/* Test shim: bsearch6 with the host compares of qsort6_cmp.c, on a buffer that the test shares with Unicorn. */
#include "../../src/recomp_rt/win/qsort6.c"
#include "../../src/recomp_rt/win/qsort6_cmp.c"
#include "../../src/recomp_rt/win/bsearch6.c"

uint8_t *g_mem;

void setmem(uint8_t *p) { g_mem = p; }

/* Searches as the original with the registers given and esp the stack pointer at entry. */
uint32_t run_bsearch6(uint32_t key, uint32_t base, uint32_t num, uint32_t width, uint32_t cmp, uint32_t esp,
                      uint32_t ebp, uint32_t ebx, uint32_t esi, uint32_t edi)
{
    bsearch6_stack st = {g_mem, esp, ebp, ebx, esi, edi};
    return bsearch6(&st, key, base, num, width, cmp);
}

int has_cmp(uint32_t cmp) { return bsearch6_supported(cmp); }
