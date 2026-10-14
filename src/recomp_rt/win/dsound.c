/* DirectSound as a silent stub (inventory 2.2): creation succeeds, a buffer keeps its data in flat
 * memory, GetStatus reports "not playing" and the other methods do nothing and succeed. */
#include "win.h"

static uint32_t ds_vtable, buffer_vtable;

/* A buffer: vtable, count, data +8, size +0xc. */
static void buf_GetStatus(void)
{
    wr32(g_mem, rt_arg(1), 0);
    rt_ret_stdcall(0, 2);
}

/* Lock(offset, bytes, &p1, &n1, &p2, &n2, flags) */
static void buf_Lock(void)
{
    uint32_t self = rt_arg(0), offset = rt_arg(1), bytes = rt_arg(2);
    uint32_t size = rd32(g_mem, self + 0xc);
    if (offset > size) offset = size;
    if (bytes > size - offset || bytes == 0) bytes = size - offset;
    wr32(g_mem, rt_arg(3), rd32(g_mem, self + 8) + offset);
    wr32(g_mem, rt_arg(4), bytes);
    if (rt_arg(5)) wr32(g_mem, rt_arg(5), 0);
    if (rt_arg(6)) wr32(g_mem, rt_arg(6), 0);
    rt_ret_stdcall(0, 8);
}

static void buf_Play(void) { rt_ret_stdcall(0, 4); }
static void buf_SetCurrentPosition(void) { rt_ret_stdcall(0, 2); }
static void buf_SetVolume(void) { rt_ret_stdcall(0, 2); }
static void buf_SetPan(void) { rt_ret_stdcall(0, 2); }
static void buf_Stop(void) { rt_ret_stdcall(0, 1); }
static void buf_Unlock(void) { rt_ret_stdcall(0, 5); }

/* The buffer's memory goes back to the heap on its last Release. */
static void buf_Release(void)
{
    uint32_t self = rt_arg(0);
    uint32_t n = rd32(g_mem, self + 4);
    if (n) n--;
    wr32(g_mem, self + 4, n);
    if (!n && rd32(g_mem, self + 8)) {
        rt_free(rd32(g_mem, self + 8));
        wr32(g_mem, self + 8, 0);
    }
    rt_ret_stdcall(n, 1);
}

static const WinMethod buffer_methods[] = {
    {0x04, "AddRef", win_com_addref}, {0x08, "Release", buf_Release}, {0x24, "GetStatus", buf_GetStatus},
    {0x2c, "Lock", buf_Lock}, {0x30, "Play", buf_Play}, {0x34, "SetCurrentPosition", buf_SetCurrentPosition},
    {0x3c, "SetVolume", buf_SetVolume}, {0x40, "SetPan", buf_SetPan}, {0x48, "Stop", buf_Stop},
    {0x4c, "Unlock", buf_Unlock}, {0, NULL, NULL},
};

/* CreateSoundBuffer(desc, &buffer, outer). DSBUFFERDESC: size, flags, bytes +8, reserved, format +0x10. */
static void ds_CreateSoundBuffer(void)
{
    uint32_t desc = rt_arg(1), out = rt_arg(2);
    uint32_t bytes = rd32(g_mem, desc + 8);
    uint32_t b = win_make_object(buffer_vtable, 16);
    wr32(g_mem, b + 8, bytes ? rt_alloc_zero(bytes) : 0);
    wr32(g_mem, b + 0xc, bytes);
    wr32(g_mem, out, b);
    rt_ret_stdcall(0, 4);
}

static void ds_SetCooperativeLevel(void) { rt_ret_stdcall(0, 3); }

static const WinMethod ds_methods[] = {
    {0x04, "AddRef", win_com_addref}, {0x08, "Release", win_com_release},
    {0x0c, "CreateSoundBuffer", ds_CreateSoundBuffer}, {0x18, "SetCooperativeLevel", ds_SetCooperativeLevel},
    {0, NULL, NULL},
};

/* DirectSoundCreate(guid, &object, outer), ordinal 1 of dsound.dll. */
RT_IMPORT(DSOUND_ord1)
{
    if (!ds_vtable) {
        ds_vtable = win_make_vtable("IDirectSound", 11, ds_methods);
        buffer_vtable = win_make_vtable("IDirectSoundBuffer", 21, buffer_methods);
    }
    wr32(g_mem, rt_arg(1), win_make_object(ds_vtable, 16));
    rt_ret_stdcall(0, 3);
}
