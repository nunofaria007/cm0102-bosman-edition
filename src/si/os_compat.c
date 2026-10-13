#include "cm_types.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include <dirent.h>
#include <errno.h>
#include <ctype.h>

bool os_file_exists(const char *path)
{
    struct stat st;
    return stat(path, &st) == 0;
}

bool os_dir_exists(const char *path)
{
    struct stat st;
    return stat(path, &st) == 0 && S_ISDIR(st.st_mode);
}

bool os_mkdir(const char *path)
{
    return mkdir(path, 0755) == 0 || errno == EEXIST;
}

u32 os_get_file_size(const char *path)
{
    struct stat st;
    if (stat(path, &st) != 0) return 0;
    return (u32)st.st_size;
}

char *os_find_file_ci(const char *dir, const char *name)
{
    DIR *d = opendir(dir);
    if (!d) return NULL;

    struct dirent *ent;
    while ((ent = readdir(d)) != NULL) {
        if (strcasecmp(ent->d_name, name) == 0) {
            size_t len = strlen(dir) + 1 + strlen(ent->d_name) + 1;
            char *result = malloc(len);
            if (result) {
                snprintf(result, len, "%s/%s", dir, ent->d_name);
            }
            closedir(d);
            return result;
        }
    }

    closedir(d);
    return NULL;
}

void os_path_to_posix(char *path)
{
    for (char *p = path; *p; p++) {
        if (*p == '\\') *p = '/';
    }
}

const char *os_get_user_data_dir(void)
{
    static char buf[512];
    const char *home = getenv("HOME");
    if (!home) home = "/tmp";
    snprintf(buf, sizeof(buf),
             "%s/Library/Application Support/cm0102", home);
    os_mkdir(buf);
    return buf;
}
