/* The bsearch of the original's CRT (MSVC 6, 0x946464) in host code, for the compares that qsort6 has.
 *
 * The original is called about 28 million times in a new game with all leagues (Shortlist_Manager
 * try_add_player_to_club_shortlist 0x842e10, add_shortlist_entry 0x8423f0, PlayerStats::get_language_fluency
 * 0x7c6830), and every step of it goes through generated code for the compare. Here the steps are the same,
 * read from its bytes (0x946464..0x9464f0): the same middle, the same compare order, the same result.
 *
 * What the original leaves behind, and what bsearch6 writes too (with esp the stack pointer at entry, the
 * word at esp being the return address, and arguments key, base, num, width, cmp at esp+4..esp+0x14):
 *   esp-4 the caller's ebp, esp-8 ebx, esp-12 esi, esp-16 edi (the prologue 0x946464, 0x94646a, 0x94646b,
 *   0x94646c). Each compare pushes the row (esp-20), the key (esp-24), then the call pushes its return address
 *   (esp-28: 0x9464a4 in the loop, 0x9464e3 for one element) and the compare pushes esi (esp-32; each compare
 *   of qsort6_cmp.c does, and no other word).
 *   It also uses its own argument words: base (esp+8) is moved up (0x9464c1) and num (esp+12) holds the
 *   odd flag of the count (0x946486, 0x94648b).
 * Not reproduced, as in qsort6: ecx and edx, which the caller of a cdecl function may not read.
 *
 * Only the compares of qsort6_cmp.c that push esi once are taken (0x7c6d40, 0x757ea0, 0x844740, 0x83b670);
 * 0x4b0810 pushes nothing, so the word at esp-32 would keep an older value, and the original is run for it.
 * A key or row at address 0 is also left to the original (the compares have a path for NULL). */
#include "qsort6.h"

#include <stdint.h>
#include <string.h>

int bsearch6_supported(uint32_t cmp)
{
    return cmp == 0x7c6d40u || cmp == 0x757ea0u || cmp == 0x844740u || cmp == 0x83b670u;
}

static void bs_put(const bsearch6_stack *st, uint32_t at, uint32_t v)
{
    memcpy(st->mem + at, &v, 4);
}

/* Gives the game address that the original leaves in eax (0 when nothing is found). */
uint32_t bsearch6(const bsearch6_stack *st, uint32_t key, uint32_t base, uint32_t num, uint32_t width,
                  uint32_t cmp)
{
    qsort6_cmp_fn fn = qsort6_host_cmp(cmp);
    const uint32_t E = st->esp;
    bs_put(st, E - 4, st->ebp);
    bs_put(st, E - 8, st->ebx);
    bs_put(st, E - 12, st->esi);
    bs_put(st, E - 16, st->edi);
    uint32_t esi = st->esi;
    uint32_t eax = num;
    uint32_t hi = (eax - 1) * width + base;                 /* 0x94646d..0x946477 */
    bs_put(st, E + 8, base);                                   /* 0x946479 */
    if (base > hi) return 0;                                /* 0x94647c: ja */
    for (;;) {
        uint32_t half = eax >> 1;                           /* 0x946480, 0x946482 */
        if (!half) {                                        /* 0x946484: je 0x9464d6, one element or none */
            if (!eax) return 0;
            bs_put(st, E - 20, base);
            bs_put(st, E - 24, key);
            bs_put(st, E - 28, 0x9464e3u);
            bs_put(st, E - 32, esi);
            int32_t r = fn(st->mem + key, st->mem + base);
            return r == 0 ? base : 0;
        }
        uint32_t odd = eax & 1;
        bs_put(st, E + 12, odd);                               /* 0x946486, 0x94648b */
        eax = odd ? half : half - 1;                        /* 0x946489, 0x946491 */
        esi = eax * width + base;                           /* 0x946494..0x94649b */
        bs_put(st, E - 20, esi);
        bs_put(st, E - 24, key);
        bs_put(st, E - 28, 0x9464a4u);
        bs_put(st, E - 32, esi);
        int32_t r = fn(st->mem + key, st->mem + esi);
        if (r == 0) return esi;                             /* 0x9464a8: je 0x9464d2 */
        if (r > 0) {                                        /* 0x9464aa: jge 0x9464bc */
            esi += width;
            base = esi;
            bs_put(st, E + 8, base);                           /* 0x9464c1 */
            eax = half;                                     /* 0x9464c4 */
        } else {
            esi -= width;                                   /* 0x9464ac */
            hi = esi;
            eax = odd ? half : half - 1;                    /* 0x9464af..0x9464ba */
        }
        if (base > hi) return 0;                            /* 0x9464c6, 0x9464c9: jbe loops */
    }
}
