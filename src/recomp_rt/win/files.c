/* KERNEL32 files and folders over the host file system.
 *
 * Paths: C:\ is the host root, so the original's folder /Users/max/Downloads/Game is
 * C:\Users\max\Downloads\Game. D:\ is the CD drive: the folder that holds the player's own cm0102.exe,
 * which the CD check looks for (inventory 4.6). A relative path starts at the current folder, which
 * WinMain sets from the registry value Location. Each part of a path is matched without regard to case,
 * as Windows does; a part that does not exist keeps its spelling (a new file). */
#include "win.h"

#include <ctype.h>
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <libgen.h>
#include <stdlib.h>
#include <strings.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>

char win_cwd[1024] = "C:\\";

void win_error_from_errno(void)
{
    switch (errno) {
    case ENOENT: win_last_error = ERROR_FILE_NOT_FOUND; break;
    case ENOTDIR: win_last_error = ERROR_PATH_NOT_FOUND; break;
    case EEXIST: win_last_error = ERROR_FILE_EXISTS; break;
    case EACCES: case EPERM: case EISDIR: win_last_error = ERROR_ACCESS_DENIED; break;
    case ENOSPC: win_last_error = 112; break;    /* ERROR_DISK_FULL */
    default: win_last_error = 31; break;          /* ERROR_GEN_FAILURE */
    }
}

/* The full Windows path of a name: drive letter, backslashes, no "." or "..", no trailing backslash
 * except after the drive. */
static void full_path(const char *in, char *out, size_t room)
{
    char buf[2048];
    char drive;
    const char *rest;
    if (isalpha((unsigned char)in[0]) && in[1] == ':') {
        drive = (char)toupper((unsigned char)in[0]);
        rest = in + 2;
        if (rest[0] != '\\' && rest[0] != '/') {
            /* A drive-relative path: relative to the current folder when on its drive, else to the root. */
            if (toupper((unsigned char)win_cwd[0]) == drive) snprintf(buf, sizeof buf, "%s\\%s", win_cwd + 2, rest);
            else snprintf(buf, sizeof buf, "\\%s", rest);
        } else {
            snprintf(buf, sizeof buf, "%s", rest);
        }
    } else if (in[0] == '\\' || in[0] == '/') {
        drive = (char)toupper((unsigned char)win_cwd[0]);
        snprintf(buf, sizeof buf, "%s", in);
    } else {
        drive = (char)toupper((unsigned char)win_cwd[0]);
        snprintf(buf, sizeof buf, "%s\\%s", win_cwd + 2, in);
    }
    /* Split into parts and resolve . and .. */
    char *parts[256];
    int n = 0;
    for (char *p = buf; *p; p++) if (*p == '/') *p = '\\';
    char *save = NULL;
    for (char *tok = strtok_r(buf, "\\", &save); tok; tok = strtok_r(NULL, "\\", &save)) {
        if (!strcmp(tok, ".")) continue;
        if (!strcmp(tok, "..")) { if (n) n--; continue; }
        if (n < 256) parts[n++] = tok;
    }
    size_t len = (size_t)snprintf(out, room, "%c:", drive);
    if (!n) { snprintf(out + len, room - len, "\\"); return; }
    for (int i = 0; i < n && len < room; i++) len += (size_t)snprintf(out + len, room - len, "\\%s", parts[i]);
}

/* The host folder of a drive's root, or NULL for a drive that does not exist. */
static const char *drive_root(char drive, char *buf, size_t room)
{
    if (drive == 'C') return "";
    if (drive == 'D') {
        snprintf(buf, room, "%s", win_exe_path);
        char *slash = strrchr(buf, '/');
        if (slash) *slash = 0;
        return buf;
    }
    return NULL;
}

int win_host_path(const char *winpath, char *out, size_t room)
{
    char full[2048], rootbuf[1024];
    full_path(winpath, full, sizeof full);
    const char *root = drive_root(full[0], rootbuf, sizeof rootbuf);
    if (!root) return 0;
    size_t len = (size_t)snprintf(out, room, "%s", root);
    char *save = NULL;
    for (char *part = strtok_r(full + 2, "\\", &save); part; part = strtok_r(NULL, "\\", &save)) {
        /* Windows drops trailing dots and spaces of a name. */
        size_t pl = strlen(part);
        while (pl > 1 && (part[pl - 1] == '.' || part[pl - 1] == ' ')) part[--pl] = 0;
        char found[256] = "";
        DIR *d = opendir(len ? out : "/");
        if (d) {
            struct dirent *e;
            while ((e = readdir(d))) {
                if (!strcmp(e->d_name, part)) { snprintf(found, sizeof found, "%s", e->d_name); break; }
                if (!found[0] && !strcasecmp(e->d_name, part)) snprintf(found, sizeof found, "%s", e->d_name);
            }
            closedir(d);
        }
        len += (size_t)snprintf(out + len, room - len, "/%s", found[0] ? found : part);
        if (len >= room) return 0;
    }
    if (!len) snprintf(out, room, "/");
    return 1;
}

void win_windows_path(const char *host, char *out, size_t room)
{
    size_t len = (size_t)snprintf(out, room, "C:%s", host);
    for (size_t i = 0; i < len && i < room; i++) if (out[i] == '/') out[i] = '\\';
    if (len == 2 && room > 3) snprintf(out, room, "C:\\");
}

/* Handles. Files are 0x100, 0x104, ...; finds are 0x20000, 0x20004, ... */
#define MAX_FILES 256
#define FILE_BASE 0x100u
static int fds[MAX_FILES];
static int fd_used[MAX_FILES];

static int fd_of(uint32_t h)
{
    if (h < FILE_BASE || h >= FILE_BASE + 4 * MAX_FILES || (h & 3)) return -1;
    uint32_t i = (h - FILE_BASE) / 4;
    return fd_used[i] ? fds[i] : -1;
}

int win_is_file_handle(uint32_t h) { return fd_of(h) >= 0; }

int win_close_file(uint32_t h)
{
    int fd = fd_of(h);
    if (fd < 0) return 0;
    close(fd);
    fd_used[(h - FILE_BASE) / 4] = 0;
    return 1;
}

#define INVALID_HANDLE 0xffffffffu
#define STD_HANDLE_BASE 0xF0000000u

static int is_std(uint32_t h) { return h >= STD_HANDLE_BASE + 10 && h <= STD_HANDLE_BASE + 12; }

/* CreateFileA(name, access, share, security, disposition, flags, template) */
RT_IMPORT(CreateFileA)
{
    const char *name = win_str(rt_arg(0));
    uint32_t access = rt_arg(1), disposition = rt_arg(4);
    char host[2048];
    if (!win_host_path(name, host, sizeof host)) {
        win_last_error = ERROR_PATH_NOT_FOUND;
        rt_ret_stdcall(INVALID_HANDLE, 7);
        return;
    }
    int flags = (access & 0x40000000u) ? ((access & 0x80000000u) ? O_RDWR : O_WRONLY) : O_RDONLY;
    switch (disposition) {
    case 1: flags |= O_CREAT | O_EXCL; break;           /* CREATE_NEW */
    case 2: flags |= O_CREAT | O_TRUNC; break;          /* CREATE_ALWAYS */
    case 3: break;                                      /* OPEN_EXISTING */
    case 4: flags |= O_CREAT; break;                    /* OPEN_ALWAYS */
    case 5: flags |= O_TRUNC; break;                    /* TRUNCATE_EXISTING */
    }
    struct stat st;
    int existed = stat(host, &st) == 0;
    if (existed && S_ISDIR(st.st_mode)) {
        win_last_error = ERROR_ACCESS_DENIED;
        rt_ret_stdcall(INVALID_HANDLE, 7);
        return;
    }
    int fd = open(host, flags, 0644);
    WIN_TRACE("CreateFileA(%s -> %s, %#x, %u) = %d\n", name, host, access, disposition, fd);
    if (fd < 0) {
        win_error_from_errno();
        rt_ret_stdcall(INVALID_HANDLE, 7);
        return;
    }
    for (uint32_t i = 0; i < MAX_FILES; i++) {
        if (!fd_used[i]) {
            fd_used[i] = 1;
            fds[i] = fd;
            win_last_error = existed && (disposition == 2 || disposition == 4) ? ERROR_ALREADY_EXISTS : 0;
            rt_ret_stdcall(FILE_BASE + 4 * i, 7);
            return;
        }
    }
    rt_fatal("too many open files");
}

/* ReadFile(handle, buffer, count, &read, overlapped) */
RT_IMPORT(ReadFile)
{
    uint32_t h = rt_arg(0), buf = rt_arg(1), count = rt_arg(2), done = rt_arg(3);
    int fd = fd_of(h);
    uint32_t got = 0;
    if (fd >= 0) {
        while (got < count) {
            ssize_t r = read(fd, g_mem + buf + got, count - got);
            if (r <= 0) break;
            got += (uint32_t)r;
        }
    } else if (!is_std(h)) {
        win_last_error = ERROR_INVALID_HANDLE;
        if (done) wr32(g_mem, done, 0);
        rt_ret_stdcall(0, 5);
        return;
    }
    if (done) wr32(g_mem, done, got);
    rt_ret_stdcall(1, 5);
}

/* WriteFile(handle, buffer, count, &written, overlapped) */
RT_IMPORT(WriteFile)
{
    uint32_t h = rt_arg(0), buf = rt_arg(1), count = rt_arg(2), done = rt_arg(3);
    int fd = fd_of(h);
    uint32_t put = 0;
    if (fd >= 0) {
        while (put < count) {
            ssize_t r = write(fd, g_mem + buf + put, count - put);
            if (r <= 0) break;
            put += (uint32_t)r;
        }
        if (put < count) win_error_from_errno();
    } else if (is_std(h)) {
        put = (uint32_t)fwrite(g_mem + buf, 1, count, stderr);
    } else {
        win_last_error = ERROR_INVALID_HANDLE;
        if (done) wr32(g_mem, done, 0);
        rt_ret_stdcall(0, 5);
        return;
    }
    if (done) wr32(g_mem, done, put);
    rt_ret_stdcall(put == count, 5);
}

/* SetFilePointer(handle, low, &high, method) */
RT_IMPORT(SetFilePointer)
{
    uint32_t h = rt_arg(0), low = rt_arg(1), high_at = rt_arg(2), method = rt_arg(3);
    int fd = fd_of(h);
    if (fd < 0) {
        win_last_error = ERROR_INVALID_HANDLE;
        rt_ret_stdcall(0xffffffffu, 4);
        return;
    }
    int64_t off = high_at ? (int64_t)((uint64_t)rd32(g_mem, high_at) << 32 | low) : (int64_t)(int32_t)low;
    off_t r = lseek(fd, off, method == 0 ? SEEK_SET : method == 1 ? SEEK_CUR : SEEK_END);
    if (r < 0) {
        win_last_error = 131;                       /* ERROR_NEGATIVE_SEEK */
        rt_ret_stdcall(0xffffffffu, 4);
        return;
    }
    if (high_at) wr32(g_mem, high_at, (uint32_t)((uint64_t)r >> 32));
    win_last_error = 0;
    rt_ret_stdcall((uint32_t)r, 4);
}

RT_IMPORT(SetEndOfFile)
{
    int fd = fd_of(rt_arg(0));
    int ok = fd >= 0 && ftruncate(fd, lseek(fd, 0, SEEK_CUR)) == 0;
    rt_ret_stdcall((uint32_t)ok, 1);
}

RT_IMPORT(FlushFileBuffers) { rt_ret_stdcall(1, 1); }

RT_IMPORT(GetFileType)
{
    uint32_t h = rt_arg(0);
    if (is_std(h)) { rt_ret_stdcall(2, 1); return; }      /* FILE_TYPE_CHAR */
    if (fd_of(h) >= 0) { rt_ret_stdcall(1, 1); return; }  /* FILE_TYPE_DISK */
    win_last_error = ERROR_INVALID_HANDLE;
    rt_ret_stdcall(0, 1);
}

RT_IMPORT(CloseHandle)
{
    uint32_t h = rt_arg(0);
    win_close_file(h);
    rt_ret_stdcall(1, 1);
}

RT_IMPORT(GetFileAttributesA)
{
    const char *name = win_str(rt_arg(0));
    char host[2048];
    struct stat st;
    if (!win_host_path(name, host, sizeof host) || stat(host, &st)) {
        win_last_error = ERROR_FILE_NOT_FOUND;
        rt_ret_stdcall(0xffffffffu, 1);
        return;
    }
    rt_ret_stdcall(S_ISDIR(st.st_mode) ? 0x10u : 0x20u, 1);
}

RT_IMPORT(DeleteFileA)
{
    const char *name = win_str(rt_arg(0));
    char host[2048];
    int ok = win_host_path(name, host, sizeof host) && unlink(host) == 0;
    WIN_TRACE("DeleteFileA(%s) = %d\n", name, ok);
    if (!ok) win_error_from_errno();
    rt_ret_stdcall((uint32_t)ok, 1);
}

RT_IMPORT(MoveFileA)
{
    char from[2048], to[2048];
    struct stat st;
    int ok = win_host_path(win_str(rt_arg(0)), from, sizeof from) && win_host_path(win_str(rt_arg(1)), to, sizeof to);
    if (ok && stat(to, &st) == 0) { win_last_error = ERROR_ALREADY_EXISTS; ok = 0; }
    else if (ok && rename(from, to)) { win_error_from_errno(); ok = 0; }
    rt_ret_stdcall((uint32_t)ok, 2);
}

RT_IMPORT(CreateDirectoryA)
{
    char host[2048];
    int ok = win_host_path(win_str(rt_arg(0)), host, sizeof host) && mkdir(host, 0755) == 0;
    if (!ok) win_last_error = errno == EEXIST ? ERROR_ALREADY_EXISTS : ERROR_PATH_NOT_FOUND;
    rt_ret_stdcall((uint32_t)ok, 2);
}

RT_IMPORT(SetCurrentDirectoryA)
{
    const char *name = win_str(rt_arg(0));
    char host[2048], full[1024];
    struct stat st;
    if (!win_host_path(name, host, sizeof host) || stat(host, &st) || !S_ISDIR(st.st_mode)) {
        win_last_error = ERROR_PATH_NOT_FOUND;
        rt_ret_stdcall(0, 1);
        return;
    }
    full_path(name, full, sizeof full);
    snprintf(win_cwd, sizeof win_cwd, "%s", full);
    WIN_TRACE("SetCurrentDirectoryA(%s) -> %s\n", name, win_cwd);
    rt_ret_stdcall(1, 1);
}

/* Returns the length, or the room needed (with the NUL) when the buffer is too small. */
RT_IMPORT(GetCurrentDirectoryA)
{
    uint32_t room = rt_arg(0), buf = rt_arg(1);
    uint32_t len = (uint32_t)strlen(win_cwd);
    if (room <= len) { rt_ret_stdcall(len + 1, 2); return; }
    win_put_str(buf, win_cwd, room);
    rt_ret_stdcall(len, 2);
}

/* GetFullPathNameA(name, room, buffer, &file part) */
RT_IMPORT(GetFullPathNameA)
{
    uint32_t room = rt_arg(1), buf = rt_arg(2), part = rt_arg(3);
    char full[1024];
    full_path(win_str(rt_arg(0)), full, sizeof full);
    uint32_t len = (uint32_t)strlen(full);
    if (room <= len) { rt_ret_stdcall(len + 1, 4); return; }
    win_put_str(buf, full, room);
    if (part) {
        char *slash = strrchr(full, '\\');
        wr32(g_mem, part, slash && slash[1] ? buf + (uint32_t)(slash + 1 - full) : 0);
    }
    rt_ret_stdcall(len, 4);
}

/* Drives: C: the fixed disk, D: the CD. */
RT_IMPORT(GetLogicalDriveStringsA)
{
    uint32_t room = rt_arg(0), buf = rt_arg(1);
    static const char drives[] = "C:\\\0D:\\\0";
    if (room < sizeof drives) { rt_ret_stdcall(sizeof drives, 2); return; }
    memcpy(g_mem + buf, drives, sizeof drives);
    rt_ret_stdcall(sizeof drives - 1, 2);
}

RT_IMPORT(GetDriveTypeA)
{
    uint32_t root = rt_arg(0);
    char drive = root ? (char)toupper((unsigned char)win_str(root)[0]) : win_cwd[0];
    rt_ret_stdcall(drive == 'C' ? 3u : drive == 'D' ? 5u : 1u, 1);   /* FIXED, CDROM, NO_ROOT_DIR */
}

/* GetDiskFreeSpaceA(root, sectors per cluster, bytes per sector, free clusters, clusters) */
RT_IMPORT(GetDiskFreeSpaceA)
{
    if (rt_arg(1)) wr32(g_mem, rt_arg(1), 8);
    if (rt_arg(2)) wr32(g_mem, rt_arg(2), 512);
    if (rt_arg(3)) wr32(g_mem, rt_arg(3), 0x200000);
    if (rt_arg(4)) wr32(g_mem, rt_arg(4), 0x400000);
    rt_ret_stdcall(1, 5);
}

/* File times. */
static uint64_t filetime_of(time_t t) { return ((uint64_t)t + 11644473600ull) * 10000000ull; }

RT_IMPORT(FileTimeToLocalFileTime)
{
    uint64_t ft = rd64(g_mem, rt_arg(0));
    wr64(g_mem, rt_arg(1), ft - (uint64_t)((int64_t)win_bias_minutes() * 600000000ll));
    rt_ret_stdcall(1, 2);
}

RT_IMPORT(FileTimeToSystemTime)
{
    uint64_t ft = rd64(g_mem, rt_arg(0));
    time_t s = (time_t)(ft / 10000000ull) - (time_t)11644473600ll;
    struct tm t;
    gmtime_r(&s, &t);
    win_put_systemtime(rt_arg(1), &t, (uint32_t)(ft / 10000ull % 1000));
    rt_ret_stdcall(1, 2);
}

/* Finds. A find holds the matching names of one folder, read at FindFirstFileA. */
/* The original does not close every find (the CD check), and Windows has no small limit. */
#define MAX_FINDS 4096
#define FIND_BASE 0x20000u
typedef struct { int used; char *dir; char **names; int count, next; } Find;
static Find finds[MAX_FINDS];

/* * and ? without regard to case. As on Windows, "*.*" matches every name, also one without a dot. */
static int glob(const char *pat, const char *s)
{
    for (; *pat; pat++, s++) {
        if (*pat == '*') {
            for (const char *t = s;; t++) {
                if (glob(pat + 1, t)) return 1;
                if (!*t) return 0;
            }
        }
        if (!*s) return 0;
        if (*pat != '?' && tolower((unsigned char)*pat) != tolower((unsigned char)*s)) return 0;
    }
    return !*s;
}

static int wild_match(const char *pat, const char *s)
{
    if (!strcmp(pat, "*.*")) return 1;
    return glob(pat, s);
}

/* WIN32_FIND_DATAA: attributes 0, times 4/0xc/0x14, size 0x1c/0x20, name 0x2c, short name 0x130. */
static void fill_find_data(Find *f, uint32_t data, const char *name)
{
    char path[2400];
    struct stat st;
    snprintf(path, sizeof path, "%s/%s", f->dir, name);
    memset(g_mem + data, 0, 0x140);
    if (stat(path, &st) == 0) {
        wr32(g_mem, data, S_ISDIR(st.st_mode) ? 0x10u : 0x20u);
        uint64_t ft = filetime_of(st.st_mtime);
        wr64(g_mem, data + 4, ft);
        wr64(g_mem, data + 0xc, ft);
        wr64(g_mem, data + 0x14, ft);
        wr32(g_mem, data + 0x1c, (uint32_t)((uint64_t)st.st_size >> 32));
        wr32(g_mem, data + 0x20, (uint32_t)st.st_size);
    }
    win_put_str(data + 0x2c, name, 260);
}

static void free_find(Find *f)
{
    for (int i = 0; i < f->count; i++) free(f->names[i]);
    free(f->names);
    free(f->dir);
    memset(f, 0, sizeof *f);
}

RT_IMPORT(FindFirstFileA)
{
    const char *pattern = win_str(rt_arg(0));
    uint32_t data = rt_arg(1);
    char full[1024], dirpart[1024], host[2048];
    full_path(pattern, full, sizeof full);
    char *slash = strrchr(full, '\\');
    const char *mask = slash + 1;
    snprintf(dirpart, sizeof dirpart, "%.*s", (int)(slash - full) + (slash == full + 2 ? 1 : 0), full);
    Find *f = NULL;
    int i;
    for (i = 0; i < MAX_FINDS; i++) if (!finds[i].used) { f = &finds[i]; break; }
    if (!f) rt_fatal("too many finds");
    DIR *d = win_host_path(dirpart, host, sizeof host) ? opendir(host) : NULL;
    if (!d) {
        win_last_error = ERROR_PATH_NOT_FOUND;
        rt_ret_stdcall(INVALID_HANDLE, 2);
        return;
    }
    f->used = 1;
    f->dir = strdup(host);
    int is_root = strlen(dirpart) <= 3;
    struct dirent *e;
    while ((e = readdir(d))) {
        if (is_root && (!strcmp(e->d_name, ".") || !strcmp(e->d_name, ".."))) continue;
        if (!wild_match(mask, e->d_name)) continue;
        f->names = realloc(f->names, sizeof(char *) * (size_t)(f->count + 1));
        f->names[f->count++] = strdup(e->d_name);
    }
    closedir(d);
    WIN_TRACE("FindFirstFileA(%s) in %s: %d matches\n", pattern, host, f->count);
    if (!f->count) {
        free_find(f);
        win_last_error = ERROR_FILE_NOT_FOUND;
        rt_ret_stdcall(INVALID_HANDLE, 2);
        return;
    }
    fill_find_data(f, data, f->names[0]);
    f->next = 1;
    rt_ret_stdcall(FIND_BASE + 4 * (uint32_t)i, 2);
}

static Find *find_of(uint32_t h)
{
    if (h < FIND_BASE || h >= FIND_BASE + 4 * MAX_FINDS || (h & 3)) return NULL;
    Find *f = &finds[(h - FIND_BASE) / 4];
    return f->used ? f : NULL;
}

RT_IMPORT(FindNextFileA)
{
    Find *f = find_of(rt_arg(0));
    if (!f || f->next >= f->count) {
        win_last_error = f ? ERROR_NO_MORE_FILES : ERROR_INVALID_HANDLE;
        rt_ret_stdcall(0, 2);
        return;
    }
    fill_find_data(f, rt_arg(1), f->names[f->next++]);
    rt_ret_stdcall(1, 2);
}

RT_IMPORT(FindClose)
{
    Find *f = find_of(rt_arg(0));
    if (f) free_find(f);
    rt_ret_stdcall(f != NULL, 1);
}
