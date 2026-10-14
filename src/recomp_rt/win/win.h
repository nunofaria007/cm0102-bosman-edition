/* Shared parts of the Windows replacements of the recompiled game (src/recomp_rt/win). Every import is
 * stdcall: a replacement reads its arguments with rt_arg and returns with rt_ret_stdcall. */
#ifndef RECOMP_WIN_H
#define RECOMP_WIN_H

#include "rt.h"
#include "rt_host.h"

#include <stdint.h>
#include <stdio.h>
#include <string.h>

/* Strings in flat memory. */
static inline char *win_str(uint32_t a) { return (char *)(g_mem + a); }
static inline void win_put_str(uint32_t dst, const char *s, uint32_t room)
{
    if (!room) return;
    size_t n = strlen(s);
    if (n >= room) n = room - 1;
    memcpy(g_mem + dst, s, n);
    g_mem[dst + n] = 0;
}

/* Settings of the run (main.c). */
extern char win_game_dir[1024];        /* host folder of the original: cwd, registry Location */
extern char win_user_dir[1024];        /* host folder of every write: ~/Library/Application Support/cm0102 */
extern char win_exe_path[1024];        /* host path of the player's cm0102.exe: the CD drive D:\ */
extern char win_command_line[1024];    /* the Windows command line: "cm0102.exe" and the options */
extern int win_trace;                  /* RECOMP_TRACE: print each import call */

/* GetLastError. */
extern uint32_t win_last_error;
#define ERROR_FILE_NOT_FOUND 2u
#define ERROR_PATH_NOT_FOUND 3u
#define ERROR_ACCESS_DENIED 5u
#define ERROR_INVALID_HANDLE 6u
#define ERROR_NO_MORE_FILES 18u
#define ERROR_ALREADY_EXISTS 183u
#define ERROR_FILE_EXISTS 80u
void win_error_from_errno(void);

/* Paths (files.c). A Windows path maps to a host path: C:\ is the host root, D:\ is the CD drive, a
 * relative path starts at the current folder. Each part is matched without regard to case. */
int win_host_path(const char *winpath, char *out, size_t room);
/* The Windows path of a host folder (C:\ plus the host path). */
void win_windows_path(const char *host, char *out, size_t room);
extern char win_cwd[1024];             /* current folder, as a Windows path */

/* Handles of files (files.c). */
int win_is_file_handle(uint32_t h);
int win_close_file(uint32_t h);

/* Memory that the runtime owns and never frees, zeroed (kernel32.c). */
uint32_t win_static_alloc(uint32_t size);

/* Time (kernel32.c): 100 ns units since 1601 from the host clock; the SYSTEMTIME of a struct tm; the
 * host's minutes behind UTC. */
struct tm;
uint64_t win_filetime_now(void);
void win_put_systemtime(uint32_t p, const struct tm *t, uint32_t ms);
int32_t win_bias_minutes(void);
/* 1 when RECOMP_FIXED_CLOCK is set (test runs): repeatable clocks, see kernel32.c. */
int win_fixed_clock(void);

/* The settings file of the user folder: the display mode sets rt_screen_w and rt_screen_h (display.c). */
void win_read_settings(void);

/* Window, input and screen (user32.c, ddraw.c). */
void win_video_init(void);
void win_present(void);                /* show the primary surface */
void win_poll_events(void);            /* SDL events to the message queue */
uint32_t win_back_buffer(void);        /* flat address of the RGB565 back buffer, or 0 */
void win_dump_screen(const char *path);
extern uint32_t win_wndproc;
extern uint32_t win_hwnd;

/* COM (com.c). An object starts with its vtable and a reference count at +4. The list of methods ends
 * with a NULL function. */
typedef struct { uint32_t offset; const char *name; RtFn fn; } WinMethod;
uint32_t win_make_vtable(const char *iface, int slots, const WinMethod *methods);
uint32_t win_make_object(uint32_t vtable, uint32_t size);
void win_com_addref(void);
void win_com_release(void);

/* Logging of import calls. */
#define WIN_TRACE(...) do { if (win_trace) fprintf(stderr, __VA_ARGS__); } while (0)

#endif
