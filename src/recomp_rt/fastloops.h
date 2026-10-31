#ifndef FASTLOOPS_H
#define FASTLOOPS_H
/* Native versions of hot loops inside functions of the original. tools/recomp/speedups.py puts a call
 * in place of the first instruction of the loop; the C code does what the loop does, with the same
 * results in the same registers and memory, and the translated code goes on after the loop. */

/* Player initialisation 0x535a80, 0x537777..0x537812: the distance of a player's attribute vector to
 * a template. The vectors are arrays of 3-byte entries (word, byte) that end in the word 0xff81:
 * g_player_init_attribute_vector_a at 0xae2044 and _b at 0xae2114. The loop (308 million turns in a game
 * creation) adds to ecx for each entry; the byte at esp+0x1b counts the entries and the byte at esp+0x36
 * decides which weight a pair of values gets. The result is in ecx; eax, edx, si and di hold what the
 * original leaves in them. bp is the low word of ebp. */
static inline void rt_player_init_distance(uint32_t esp, uint32_t ebp, uint32_t *peax, uint32_t *pecx,
                                           uint32_t *pedx, uint32_t *pesi, uint32_t *pedi)
{
    const uint8_t *m = g_mem;
    const uint32_t A = 0xae2044u, B = 0xae2114u;
    uint32_t ecx = *pecx, edx = *pedx, esi = *pesi, edi = *pedi, eax = 0;
    uint8_t cnt = m[esp + 0x1b];
    const int32_t sides = (int8_t)m[esp + 0x36];
    const int32_t bp = (int16_t)(ebp & 0xffffu);
    for (;;) {
        int32_t di = (int16_t)rd16(m, B + eax);
        edi = (edi & 0xffff0000u) | (uint32_t)(di & 0xffff);
        if (di < 1 || di > 0x14) {
            ecx += 100;
        } else {
            int32_t si = (int16_t)rd16(m, A + eax);
            esi = (esi & 0xffff0000u) | (uint32_t)(si & 0xffff);
            if (si >= 1 && si <= 0x14) {
                int32_t d;
                if (cnt == 0) {
                    d = bp - (int32_t)(int16_t)rd16(m, B);
                    edx = (uint32_t)(d >> 31);
                    ecx += (uint32_t)((d ^ (d >> 31)) - (d >> 31)) * 1000u;
                } else {
                    d = si - di;
                    edx = (uint32_t)(d >> 31);
                    uint32_t ad = (uint32_t)((d ^ (d >> 31)) - (d >> 31));
                    ecx += (int8_t)cnt >= sides ? ad << 4 : ad * 25u;
                }
            }
        }
        cnt = (uint8_t)(cnt + 1);
        eax = (uint32_t)((int32_t)(int8_t)cnt * 3);
        if (rd16(m, A + eax) == 0xff81u) break;
    }
    g_mem[esp + 0x1b] = cnt;
    *peax = eax; *pecx = ecx; *pedx = edx; *pesi = esi; *pedi = edi;
}

#endif
