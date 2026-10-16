/* The tactic list of the Formation drop-down of Best XI (views.h). Research:
 * docs/research/2026-10-16-tactics-inventory.md.
 *
 * The game's own Tactics drop-down (the "Set To Default" items of the Tactics button on the tactics screen,
 * 0x89a3a7) lists the preset tactics of the store [0xde1d08]: the count at +0x5fd (17) and the blocks of
 * 0x18e3 bytes from the pointer at +0x5f9, in the order of the load 0x893dc0 (4-4-2 first). Each item shows
 * the block name at +0. The game loads the presets once, at the start (0x82a9c0), from the .pct files of
 * the Data folder; the runtime reads the blocks in memory, as 0x896570 does, so the list holds any change
 * that the load makes. The 28 other blocks of the store (+0x5f1, the computer managers' tactics) are not in
 * the drop-down, and so not in this list. The drop-down's recent tactic files of the manager are left out.
 *
 * The list starts with the manager's current tactic (0x5fbd00), which the game's drop-down does not hold:
 * Best XI opens on it (Q103). A computer manager has no tactic block there (0x5fbd00 shows an error box for
 * him), so the list is always the human manager's.
 *
 * Only reads: no allocation, no write. */
#include "win.h"
#include "views.h"

#include <stdio.h>

#define HUMANS 0xb63c98u         /* this of 0x5fbd00 */
#define STORE 0xde1d08u          /* the tactic store: count +0x5fd, blocks +0x5f9 */
#define BLOCK 0x18e3u
#define MASKS 0x12du             /* block: 11 u16 position masks */

static uint32_t call_this(uint32_t fn, uint32_t self, int n, const uint32_t *args)
{
    Cpu saved = cpu;
    cpu.ecx = self;
    uint32_t r = rt_callback(fn, n, args);
    cpu = saved;
    return r;
}

static void from_block(uint32_t block, const char *prefix, Tactic *t)
{
    snprintf(t->name, sizeof t->name, "%s%s", prefix, (const char *)(g_mem + block));
    for (uint32_t s = 0; s < 11; s++) t->masks[s] = rd16(g_mem, block + MASKS + 2 * s);
}

int tactics_list(uint32_t manager, Tactic *out, int max)
{
    uint32_t m[1] = {manager};
    uint32_t current = manager ? call_this(0x5fbd00u, rd32(g_mem, HUMANS), 1, m) : 0;
    if (!current || max < 1) return 0;
    from_block(current, "Current: ", &out[0]);
    int n = 1;
    uint32_t store = rd32(g_mem, STORE);
    int count = store ? (int32_t)rd32(g_mem, store + 0x5fd) : 0;
    uint32_t base = store ? rd32(g_mem, store + 0x5f9) : 0;
    for (int i = 0; base && i < count && n < max; i++) from_block(base + (uint32_t)i * BLOCK, "", &out[n++]);
    return n;
}
