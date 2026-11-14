/* Test shim: qsort6 with the host compares of qsort6_cmp.c, on a buffer that the test shares with Unicorn. */
#include "../../src/recomp_rt/win/qsort6.c"
#include "../../src/recomp_rt/win/qsort6_cmp.c"

uint8_t *g_mem;

void setmem(uint8_t *p) { g_mem = p; }

/* Sorts num rows at game address base as the original with ebp = esp - 4 and the given registers. Gives the
 * game address that the original leaves in eax, or 0 when it leaves eax unchanged. */
uint32_t run_qsort6(uint32_t base, uint32_t num, uint32_t width, uint32_t cmp, uint32_t esp, uint32_t ebp,
                    uint32_t ebx, uint32_t esi, uint32_t edi)
{
    qsort6_stack st = {g_mem, esp - 4, base, ebp, ebx, esi, edi, cmp, 0, qsort6_host_cmp(cmp)};
    uint8_t *eax = qsort6(g_mem + base, num, width, &st);
    return eax ? (uint32_t)(eax - g_mem) : 0;
}

int has_cmp(uint32_t cmp) { return qsort6_host_cmp(cmp) != NULL; }
