/* CRT functions of the original that the runtime replaces by address. The rest of the CRT is
 * recompiled with the game.
 *
 * memcpy (0x94ab50) and memmove (0x94f0f0) are the same MSVC 6 code. It jumps through tables with
 * indexes that the code does not bound (and one table read at negative indexes), which the recompiler
 * cannot follow. Both copy as memmove does and return the target. */
#include "win.h"

RT_REPLACE(0094ab50, memcpy)
{
    uint32_t dst = rt_arg(0), src = rt_arg(1), n = rt_arg(2);
    memmove(g_mem + dst, g_mem + src, n);
    rt_ret_cdecl(dst);
}

RT_REPLACE(0094f0f0, memmove)
{
    uint32_t dst = rt_arg(0), src = rt_arg(1), n = rt_arg(2);
    memmove(g_mem + dst, g_mem + src, n);
    rt_ret_cdecl(dst);
}
