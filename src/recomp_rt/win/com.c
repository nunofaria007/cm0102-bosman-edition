/* COM objects for the original: a vtable in flat memory whose slots hold fake addresses of host
 * functions (rt_host_fn). A slot without a replacement stops the program with the interface and the
 * offset of the method. */
#include "win.h"

#include <stdlib.h>

static void missing_method(void)
{
    rt_fatal("COM method %s has no replacement (called from %#x)", rt_name_of(rt_call_target), rd32(g_mem, cpu.esp));
}

uint32_t win_make_vtable(const char *iface, int slots, const WinMethod *methods)
{
    uint32_t vt = win_static_alloc(4u * (uint32_t)slots);
    for (int i = 0; i < slots; i++) {
        char *name = malloc(64);
        snprintf(name, 64, "%s+%#x", iface, 4 * i);
        wr32(g_mem, vt + 4u * (uint32_t)i, rt_host_fn(name, missing_method));
    }
    for (const WinMethod *m = methods; m->fn; m++) {
        if (m->offset / 4 >= (uint32_t)slots) rt_fatal("%s: method offset %#x out of range", iface, m->offset);
        char *name = malloc(64);
        snprintf(name, 64, "%s::%s", iface, m->name);
        wr32(g_mem, vt + m->offset, rt_host_fn(name, m->fn));
    }
    return vt;
}

/* AddRef and Release on the count at +4 of every object; the objects are never freed. */
void win_com_addref(void)
{
    uint32_t self = rt_arg(0);
    uint32_t n = rd32(g_mem, self + 4) + 1;
    wr32(g_mem, self + 4, n);
    rt_ret_stdcall(n, 1);
}

void win_com_release(void)
{
    uint32_t self = rt_arg(0);
    uint32_t n = rd32(g_mem, self + 4);
    if (n) n--;
    wr32(g_mem, self + 4, n);
    rt_ret_stdcall(n, 1);
}

uint32_t win_make_object(uint32_t vtable, uint32_t size)
{
    uint32_t obj = win_static_alloc(size < 8 ? 8 : size);
    wr32(g_mem, obj, vtable);
    wr32(g_mem, obj + 4, 1);
    return obj;
}
