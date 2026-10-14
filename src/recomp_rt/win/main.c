/* cm0102_recomp: runs the recompiled original from its entry point, WinMainCRTStartup (0x947a13).
 *
 * Usage: cm0102_recomp [--exe <cm0102.exe>] [options of the original, for example -windowed]
 * The exe is the player's own cm0102.exe; its folder is the game folder (Data, Pictures, ...), which
 * the registry value Location gives to WinMain, and the CD drive D:\ (files.c).
 * Environment: CM0102_EXE (the exe), RECOMP_TRACE=1 (print import calls), RECOMP_SCRIPT and RECOMP_DUMP
 * (user32.c), RECOMP_NO_BOX=1 (message boxes only on stderr). */
#include "win.h"

#include <SDL.h>
#include <stdlib.h>

#ifndef CM0102_DEFAULT_EXE
#define CM0102_DEFAULT_EXE "cm0102.exe"
#endif

char win_game_dir[1024];
char win_exe_path[1024];
char win_command_line[1024];
int win_trace;

/* The game's stack: 16 MB below the runtime's memory. */
#define GAME_STACK_BASE 0x10000000u
#define GAME_STACK_SIZE 0x01000000u
#define GAME_STACK_TOP (GAME_STACK_BASE + GAME_STACK_SIZE - 0x100u)

void f_00947a13(void);

static void at_end(void)
{
    const char *dump = getenv("RECOMP_DUMP");
    if (dump) win_dump_screen(dump);
}

int main(int argc, char **argv)
{
    const char *exe = getenv("CM0102_EXE") ? getenv("CM0102_EXE") : CM0102_DEFAULT_EXE;
    int first = 1;
    if (argc > 2 && !strcmp(argv[1], "--exe")) { exe = argv[2]; first = 3; }
    if (!realpath(exe, win_exe_path)) {
        fprintf(stderr, "cm0102_recomp: cannot find %s. Give the path of your cm0102.exe with --exe.\n", exe);
        return 2;
    }
    snprintf(win_game_dir, sizeof win_game_dir, "%s", win_exe_path);
    char *slash = strrchr(win_game_dir, '/');
    if (slash) *slash = 0;
    win_trace = getenv("RECOMP_TRACE") && *getenv("RECOMP_TRACE");

    /* The command line as Windows gives it: the quoted program path, then the options. */
    char exe_win[1100];
    win_windows_path(win_exe_path, exe_win, sizeof exe_win);
    size_t len = (size_t)snprintf(win_command_line, sizeof win_command_line, "\"%s\"", exe_win);
    for (int i = first; i < argc && len < sizeof win_command_line; i++)
        len += (size_t)snprintf(win_command_line + len, sizeof win_command_line - len, " %s", argv[i]);

    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS)) {
        fprintf(stderr, "cm0102_recomp: SDL_Init: %s\n", SDL_GetError());
        return 1;
    }
    rt_heap_bump = 0;
    rt_init(win_exe_path);
    rt_on_fatal = at_end;

    /* The current folder starts as the game folder, as for a shortcut to the game. */
    win_windows_path(win_game_dir, win_cwd, sizeof win_cwd);

    /* The thread information block at fs:0: the exception chain is empty, then the stack limits. */
    wr32(g_mem, RT_FS_BASE, 0xffffffffu);
    wr32(g_mem, RT_FS_BASE + 4, GAME_STACK_BASE + GAME_STACK_SIZE);
    wr32(g_mem, RT_FS_BASE + 8, GAME_STACK_BASE);
    wr32(g_mem, RT_FS_BASE + 0x18, RT_FS_BASE);

    cpu.esp = GAME_STACK_TOP;
    cpu.esp -= 4;
    wr32(g_mem, cpu.esp, RT_RETURN_ADDR);
    f_00947a13();
    fprintf(stderr, "cm0102_recomp: the entry point returned %u\n", cpu.eax);
    at_end();
    SDL_Quit();
    return (int)cpu.eax;
}
