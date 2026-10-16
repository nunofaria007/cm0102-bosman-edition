/* Custom data set folders (dbfolders.h). */
#include "dbfolders.h"
#include "../host.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>

static const char *const data_files[] = {
    "city.dat", "club.dat", "club_comp.dat", "club_comp_history.dat", "colour.dat", "common_names.dat",
    "continent.dat", "first_names.dat", "index.dat", "nat_club.dat", "nation.dat", "nation_comp.dat",
    "nation_comp_history.dat", "officials.dat", "second_names.dat", "stadium.dat", "staff.dat",
    "staff_comp.dat", "staff_comp_history.dat", "staff_history.dat"};
enum { FILE_COUNT = sizeof data_files / sizeof *data_files };

int dbfolders_valid(const char *dir)
{
    int seen[FILE_COUNT] = {0}, count = 0;
    HostDir *d = host_opendir(dir);
    if (!d) return 0;
    const char *e;
    while ((e = host_readdir(d))) {
        for (int i = 0; i < FILE_COUNT; i++) {
            if (!seen[i] && !strcasecmp(e, data_files[i])) {
                seen[i] = 1;
                count++;
                break;
            }
        }
    }
    host_closedir(d);
    return count == FILE_COUNT;
}

static int by_name(const void *a, const void *b)
{
    int c = strcasecmp((const char *)a, (const char *)b);
    return c ? c : strcmp((const char *)a, (const char *)b);
}

int dbfolders_list(const char *save_dir, char names[][DBFOLDER_NAME], int max)
{
    char dbs[2048], path[2400];
    snprintf(dbs, sizeof dbs, "%s/Databases", save_dir);
    HostDir *d = host_opendir(dbs);
    if (!d) return 0;
    int n = 0;
    const char *e;
    while ((e = host_readdir(d))) {
        if (e[0] == '.' || strlen(e) >= DBFOLDER_NAME) continue;
        snprintf(path, sizeof path, "%s/%s", dbs, e);
        HostStat st;
        if (host_stat(path, &st) != 0 || !st.is_dir || !dbfolders_valid(path)) continue;
        if (n < max) {
            snprintf(names[n++], DBFOLDER_NAME, "%s", e);
            continue;
        }
        /* The list is full: the folders that sort first stay, whatever order the disk gives. */
        int last = 0;
        for (int i = 1; i < n; i++)
            if (by_name(names[i], names[last]) > 0) last = i;
        if (n > 0 && by_name(e, names[last]) < 0) snprintf(names[last], DBFOLDER_NAME, "%s", e);
    }
    host_closedir(d);
    qsort(names, (size_t)n, DBFOLDER_NAME, by_name);
    return n;
}

int dbfolders_path(const char *save_dir, const char *name, char *out, size_t room)
{
    if (!name[0] || name[0] == '.' || strchr(name, '/') || strchr(name, '\\') || strlen(name) >= DBFOLDER_NAME)
        return 0;
    char path[2400];
    snprintf(path, sizeof path, "%s/Databases/%s", save_dir, name);
    HostStat st;
    if (host_stat(path, &st) != 0 || !st.is_dir || !dbfolders_valid(path)) return 0;
    if (strlen(path) >= room) return 0;
    snprintf(out, room, "%s", path);
    return 1;
}

/* The code point of code page 1252 byte 0x80 to 0x9f, in the order of the byte; 0 when none. */
static const unsigned short high[32] = {
    0x20ac, 0, 0x201a, 0x0192, 0x201e, 0x2026, 0x2020, 0x2021, 0x02c6, 0x2030, 0x0160, 0x2039, 0x0152, 0, 0x017d, 0,
    0, 0x2018, 0x2019, 0x201c, 0x201d, 0x2022, 0x2013, 0x2014, 0x02dc, 0x2122, 0x0161, 0x203a, 0x0153, 0, 0x017e, 0x0178};

void dbfolders_label(const char *utf8, char *out, size_t room)
{
    size_t n = 0;
    const unsigned char *p = (const unsigned char *)utf8;
    while (*p && n + 1 < room) {
        unsigned cp;
        int len;
        if (*p < 0x80) { cp = *p; len = 1; }
        else if ((*p & 0xe0) == 0xc0) { cp = *p & 0x1f; len = 2; }
        else if ((*p & 0xf0) == 0xe0) { cp = *p & 0x0f; len = 3; }
        else if ((*p & 0xf8) == 0xf0) { cp = *p & 0x07; len = 4; }
        else { cp = 0xfffd; len = 1; }
        for (int i = 1; i < len; i++) {
            if ((p[i] & 0xc0) != 0x80) { cp = 0xfffd; len = i; break; }
            cp = cp << 6 | (p[i] & 0x3f);
        }
        p += len;
        int byte = '?';
        if (cp >= 0x20 && cp < 0x7f) byte = (int)cp;
        else if (cp >= 0xa0 && cp <= 0xff) byte = (int)cp;
        else for (int i = 0; i < 32; i++) if (high[i] == cp && cp) byte = 0x80 + i;
        out[n++] = (char)byte;
    }
    out[n] = 0;
}
