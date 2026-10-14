/* The host functions of host.h on macOS (POSIX). */
#include "../host.h"

#include <dirent.h>
#include <fcntl.h>
#include <limits.h>
#include <mach-o/dyld.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <unistd.h>

void *host_reserve(size_t size)
{
    void *m = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANON | MAP_NORESERVE, -1, 0);
    return m == MAP_FAILED ? NULL : m;
}

int host_open(const char *path, int flags, int mode) { return open(path, flags, mode); }
long host_read(int fd, void *buf, uint32_t n) { return (long)read(fd, buf, n); }
long host_write(int fd, const void *buf, uint32_t n) { return (long)write(fd, buf, n); }
int64_t host_seek(int fd, int64_t off, int whence) { return (int64_t)lseek(fd, (off_t)off, whence); }
int host_truncate(int fd, int64_t size) { return ftruncate(fd, (off_t)size); }
int host_close(int fd) { return close(fd); }

int host_stat(const char *path, HostStat *out)
{
    struct stat st;
    if (stat(path, &st)) return -1;
    out->is_dir = S_ISDIR(st.st_mode);
    out->size = (uint64_t)st.st_size;
    out->mtime = (int64_t)st.st_mtime;
    return 0;
}

int host_mkdir(const char *path) { return mkdir(path, 0755); }
int host_unlink(const char *path) { return unlink(path); }
int host_rename(const char *from, const char *to) { return rename(from, to); }

HostDir *host_opendir(const char *path) { return (HostDir *)opendir(path); }

const char *host_readdir(HostDir *d)
{
    struct dirent *e = readdir((DIR *)d);
    return e ? e->d_name : NULL;
}

void host_closedir(HostDir *d) { closedir((DIR *)d); }

int host_realpath(const char *path, char *out, size_t room)
{
    char buf[PATH_MAX];
    if (!realpath(path, buf)) return 0;
    return (size_t)snprintf(out, room, "%s", buf) < room;
}

int host_program_path(char *out, size_t room)
{
    char exe[PATH_MAX];
    uint32_t n = sizeof exe;
    if (_NSGetExecutablePath(exe, &n)) return 0;
    return host_realpath(exe, out, room);
}

int host_current_dir(char *out, size_t room) { return getcwd(out, room) != NULL; }

int host_user_dir(char *out, size_t room)
{
    const char *home = getenv("HOME");
    if (!home || !*home) {
        fprintf(stderr, "cm0102_recomp: HOME is not set, so the user folder is unknown.\n");
        return 0;
    }
    return (size_t)snprintf(out, room, "%s/Library/Application Support/cm0102", home) < room;
}

char host_cd_drive(void) { return 'D'; }
int host_has_drive(char letter) { (void)letter; return 0; }

void host_gmtime(time_t t, struct tm *out) { gmtime_r(&t, out); }
void host_localtime(time_t t, struct tm *out) { localtime_r(&t, out); }

int32_t host_bias_minutes(void)
{
    time_t now = time(NULL);
    struct tm t;
    localtime_r(&now, &t);
    return (int32_t)(-t.tm_gmtoff / 60);
}

uint64_t host_unix_time_us(void)
{
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (uint64_t)tv.tv_sec * 1000000u + (uint64_t)tv.tv_usec;
}

FILE *host_fopen(const char *utf8_path, const char *mode)
{
    return fopen(utf8_path, mode);
}
