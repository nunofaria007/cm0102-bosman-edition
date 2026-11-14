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

/* 0x4b0810 compare_clubs_by_reputation_asc_nulls_last. The rows are pointers to clubs; the reputation is the
 * signed word at club + 0x80 (0x4b0830, 0x4b0837). A NULL club in the first row gives 1 when the second
 * is not NULL, else 0 (setnz at 0x4b0822); a NULL second club -1 (0x4b082c). */
static int cmp_club_reputation(const uint8_t *a, const uint8_t *b)
{
    uint32_t x = u32(a), y = u32(b);
    if (!x) return y != 0;
    if (!y) return -1;
    int32_t rx = (int16_t)(g_mem[x + 0x80] | g_mem[x + 0x81] << 8);
    int32_t ry = (int16_t)(g_mem[y + 0x80] | g_mem[y + 0x81] << 8);
    return rx - ry;
}

/* 0x757ea0 compare_id_pair_keys. The ids at +0, then the ids at +4 where -1 in either row counts as equal
 * (0x757ec8, 0x757ed0). The arguments are the rows; NULL is never a row of qsort. */
static int cmp_id_pair(const uint8_t *a, const uint8_t *b)
{
    uint32_t x = u32(a), y = u32(b);
    if (x != y) return (int32_t)(x - y);
    uint32_t x4 = u32(a + 4), y4 = u32(b + 4);
    if (x4 == 0xffffffffu || y4 == 0xffffffffu) return 0;
    return (int32_t)(x4 - y4);
}

/* 0x844740 compare_id_pair_keys_by_second_id: the same with the ids at +4 first, then those at +0. */
static int cmp_id_pair_second(const uint8_t *a, const uint8_t *b)
{
    uint32_t x = u32(a + 4), y = u32(b + 4);
    if (x != y) return (int32_t)(x - y);
    uint32_t x0 = u32(a), y0 = u32(b);
    if (x0 == 0xffffffffu || y0 == 0xffffffffu) return 0;
    return (int32_t)(x0 - y0);
}

/* 0x83b670 shortlist_compare_by_value_then_key: the dword at +8, then the dword at +4. */
static int cmp_shortlist(const uint8_t *a, const uint8_t *b)
{
    uint32_t d = u32(a + 8) - u32(b + 8);
    if (d) return (int32_t)d;
    return (int32_t)(u32(a + 4) - u32(b + 4));
}

qsort6_cmp_fn qsort6_host_cmp(uint32_t cmp)
{
    switch (cmp) {
    case 0x7c6d40u: return qsort6_history_cmp;
    case 0x4b0810u: return cmp_club_reputation;
    case 0x757ea0u: return cmp_id_pair;
    case 0x844740u: return cmp_id_pair_second;
    case 0x83b670u: return cmp_shortlist;
    default: return NULL;
    }
}
