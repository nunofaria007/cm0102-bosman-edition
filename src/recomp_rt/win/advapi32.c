/* ADVAPI32: the registry key that WinMain reads (inventory 4.4). */
#include "win.h"

#include <strings.h>

/* Registry: one key with one value, Location, the folder of the original (inventory 4.4). */
#define HKEY_LOCAL_MACHINE 0x80000002u
#define GAME_KEY 0x00005000u

RT_IMPORT(RegOpenKeyExA)
{
    uint32_t root = rt_arg(0), sub = rt_arg(1), result = rt_arg(4);
    const char *name = sub ? win_str(sub) : "";
    WIN_TRACE("RegOpenKeyExA(%#x, %s)\n", root, name);
    if (root == HKEY_LOCAL_MACHINE && !strcasecmp(name, "Software\\Sports Interactive Ltd\\Championship Manager 01-02")) {
        wr32(g_mem, result, GAME_KEY);
        rt_ret_stdcall(0, 5);
        return;
    }
    rt_ret_stdcall(ERROR_FILE_NOT_FOUND, 5);
}

/* RegQueryValueExA(key, name, reserved, &type, data, &size) */
RT_IMPORT(RegQueryValueExA)
{
    uint32_t key = rt_arg(0), name = rt_arg(1), type = rt_arg(3), data = rt_arg(4), size = rt_arg(5);
    if (key != GAME_KEY || !name || strcasecmp(win_str(name), "Location")) {
        rt_ret_stdcall(ERROR_FILE_NOT_FOUND, 6);
        return;
    }
    char path[1100];
    win_windows_path(win_game_dir, path, sizeof path);
    uint32_t need = (uint32_t)strlen(path) + 1;
    if (type) wr32(g_mem, type, 1);                /* REG_SZ */
    if (data && size && rd32(g_mem, size) < need) {
        wr32(g_mem, size, need);
        rt_ret_stdcall(234, 6);                    /* ERROR_MORE_DATA */
        return;
    }
    if (data) win_put_str(data, path, need);
    if (size) wr32(g_mem, size, need);
    rt_ret_stdcall(0, 6);
}
