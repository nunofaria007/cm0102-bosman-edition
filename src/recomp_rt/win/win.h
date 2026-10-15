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
/* The option database changed: the next file open finds the data set folder again (files.c). */
void win_data_set_changed(void);

/* Sound (dsound.c): 0 mutes every sound while the window lacks focus, 1 makes them heard again. */
void win_sound_focus(int focused);

/* Window, input and screen (user32.c, ddraw.c). */
void win_video_init(void);
/* Stores the window size, place and full screen state in window.txt (option remember_window). Call it at quit. */
void win_save_window(void);
void win_present(void);                /* show the primary surface */
void win_poll_events(void);            /* SDL events to the message queue */
uint32_t win_back_buffer(void);        /* flat address of the RGB565 back buffer, or 0 */
void win_dump_screen(const char *path);
extern uint32_t win_wndproc;

/* Places on the screen of the game for the keys (keys.c). Each gives the place to click in *x, *y and
 * returns 1, or returns 0 when the key has nothing to press now. */
extern uint32_t keys_display;                  /* the screen manager of the last input poll */
int keys_typing(void);                         /* 1 while a text entry takes keys */
int keys_continue_game(int *x, int *y);        /* Continue Game in the left bar */
int keys_back(int *x, int *y);                 /* the Back arrow in the left bar */
int keys_outside_popup(int *x, int *y);        /* a place outside the open popup */
int keys_left_bar_label(const char *prefix);   /* a left bar button with this text (waits.c) */
void keys_left_bar_labels(char *out, size_t size); /* the texts of the left bar buttons, for the log */
extern uint32_t win_hwnd;

/* Holiday until next match (holiday_next.c). */
int holiday_widget_before(void);               /* at each widget create: 1 for the Go on Holiday item */
void holiday_widget_after(void);               /* after that create: adds the new item */
void holiday_next_turn(void);                  /* at each human turn, before the original */
/* Faster commentary (commentary.c): at each widget create, before the rect is scaled. */
void commentary_widget_before(void);

/* The wait step of the script (waits.c). */
int waits_parse(const char *name, int *kind, uint32_t *value);   /* 0: no such state */
int waits_reached(int kind, uint32_t value, uint32_t since);    /* since: waits_poll_count() at the start */
uint32_t waits_poll_count(void);
void waits_describe(char *out, size_t size);
void waits_poll(void);                         /* at each input poll */

/* COM (com.c). An object starts with its vtable and a reference count at +4. The list of methods ends
 * with a NULL function. */
typedef struct { uint32_t offset; const char *name; RtFn fn; } WinMethod;
uint32_t win_make_vtable(const char *iface, int slots, const WinMethod *methods);
uint32_t win_make_object(uint32_t vtable, uint32_t size);
void win_com_addref(void);
void win_com_release(void);

/* Logging of import calls. */
#define WIN_TRACE(...) do { if (win_trace) fprintf(stderr, __VA_ARGS__); } while (0)

/* Clickable names and cursors (names.c). The SDL types stay out of this header. */
struct SDL_Renderer;
struct SDL_Surface;
int win_cursor_shape(uint32_t handle);          /* an SDL_SystemCursor for a cursor handle of the game */
void names_after_poll(void);                    /* after each input poll: the name under the mouse */
void names_draw(struct SDL_Renderer *renderer); /* the line under it, on the shown frame */
void names_draw_surface(struct SDL_Surface *s); /* the same line on a screen dump */

#endif
