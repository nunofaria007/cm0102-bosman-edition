#include "utils.h"
#include "scrman.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

extern char *os_find_file_ci(const char *dir, const char *name);

const char *g_pictures_dir = "./Pictures";

const char *utils_path(const char *folder, const char *name)
{
    static char path[1024];
    char part[260];
    const char *base = strcmp(folder, "CM3_PICTURES") ? g_data_dir
                                                      : g_pictures_dir;

    snprintf(path, sizeof(path), "%s", base);
    while (*name) {
        size_t length = strcspn(name, "\\/");
        char *found;

        snprintf(part, sizeof(part), "%.*s", (int)length, name);
        found = os_find_file_ci(path, part);
        if (found) {
            snprintf(path, sizeof(path), "%s", found);
            free(found);
        } else {
            size_t used = strlen(path);

            snprintf(path + used, sizeof(path) - used, "/%s", part);
        }
        name += length;
        if (*name) name++;
    }
    return path;
}
