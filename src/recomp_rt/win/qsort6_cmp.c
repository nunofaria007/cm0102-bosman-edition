/* Host copies of compares that the original's qsort 0x945333 calls, for qsort6 (qsort6.c). Each is read from
 * the code of the original and returns the same int; a and b point at the rows, as the compare is called.
 *
 * Stack: qsort6 leaves out the stack words of the compares (see compare() in qsort6.c) because every loop of
 * compares ends with pushes to the four words that a compare uses (its two arguments, its return address and
 * the esi of 0x7c6d40) before any code reads them. That holds for a compare that pushes at most esi and
 * keeps nothing else on the stack: 0x757ea0, 0x844740 and 0x83b670 push esi once (0x757ea4, 0x844744,
 * 0x83b678), 0x4b0810 pushes nothing. None has a local or stores below esp.
 *
 * Int arithmetic is done as 32-bit wrap-around, as the sub of the original. */
#include "qsort6.h"

#include <stdint.h>
#include <string.h>

extern uint8_t *g_mem;

static inline uint32_t u32(const uint8_t *p)
{
    uint32_t v;
    memcpy(&v, p, 4);
    return v;
}

qsort6_cmp_fn qsort6_host_cmp(uint32_t cmp)
{
    switch (cmp) {
    case 0x7c6d40u: return qsort6_history_cmp;
    default: return NULL;
    }
}
