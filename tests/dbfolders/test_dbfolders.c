/* Test of the custom data set folders (src/recomp_rt/win/dbfolders.c).
 * Usage: dbfolders_test   (it makes and removes its own temporary folder) */
#include "dbfolders.h"
#include "../../src/recomp_rt/host.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

static int failures;
#define CHECK(cond) do { if (!(cond)) { printf("FAIL line %d: %s\n", __LINE__, #cond); failures++; } } while (0)

static const char *const files[] = {
    "city.dat", "club.dat", "club_comp.dat", "club_comp_history.dat", "colour.dat", "common_names.dat",
    "continent.dat", "first_names.dat", "index.dat", "nat_club.dat", "nation.dat", "nation_comp.dat",
    "nation_comp_history.dat", "officials.dat", "second_names.dat", "stadium.dat", "staff.dat",
    "staff_comp.dat", "staff_comp_history.dat", "staff_history.dat"};

static char root[1024];

static void touch(const char *dir, const char *name)
{
    char p[2048];
    snprintf(p, sizeof p, "%s/%s", dir, name);
    FILE *f = fopen(p, "w");
    if (f) { fputs("x", f); fclose(f); }
}

/* A folder under root with every data file except the one named skip (or none). */
static void make_set(const char *name, const char *skip, int upper)
{
    char d[2048];
    snprintf(d, sizeof d, "%s/%s", root, name);
    mkdir(d, 0755);
    for (size_t i = 0; i < sizeof files / sizeof *files; i++) {
        if (skip && !strcmp(skip, files[i])) continue;
        char n[64];
        snprintf(n, sizeof n, "%s", files[i]);
        if (upper) for (char *c = n; *c; c++) if (*c >= 'a' && *c <= 'z') *c -= 32;
        touch(d, n);
    }
}

static int has(char names[][DBFOLDER_NAME], int n, const char *want)
{
    for (int i = 0; i < n; i++) if (!strcmp(names[i], want)) return 1;
    return 0;
}

int main(void)
{
    char tmpl[] = "/tmp/dbfolders_test_XXXXXX";
    if (!mkdtemp(tmpl)) return 2;
    snprintf(root, sizeof root, "%s", tmpl);
    char dbs[1100];
    snprintf(dbs, sizeof dbs, "%s/Databases", root);
    mkdir(dbs, 0755);
    snprintf(root, sizeof root, "%s", dbs);

    make_set("May 2026", NULL, 0);                 /* spaces */
    make_set("Zo\xc3\xab Update", NULL, 0);        /* a non-ASCII letter (UTF-8) */
    make_set("Upper", NULL, 1);                    /* file names in capitals */
    make_set("No staff", "staff.dat", 0);          /* a data file is missing */
    char d[2048];
    snprintf(d, sizeof d, "%s/Empty", dbs);
    mkdir(d, 0755);
    snprintf(d, sizeof d, "%s/Notes", dbs);
    mkdir(d, 0755);
    touch(d, "readme.txt");
    touch(dbs, "stray.dat");                       /* a file, not a folder */

    char names[DBFOLDER_MAX][DBFOLDER_NAME];
    int n = dbfolders_list(tmpl, names, DBFOLDER_MAX);
    CHECK(n == 3);
    CHECK(has(names, n, "May 2026"));
    CHECK(has(names, n, "Zo\xc3\xab Update"));
    CHECK(has(names, n, "Upper"));
    CHECK(!has(names, n, "No staff"));
    CHECK(!has(names, n, "Empty"));
    CHECK(!has(names, n, "Notes"));
    CHECK(!has(names, n, "stray.dat"));
    /* The list is sorted, ignoring case. */
    CHECK(n == 3 && !strcmp(names[0], "May 2026") && !strcmp(names[1], "Upper"));

    CHECK(dbfolders_valid(dbs) == 0);
    snprintf(d, sizeof d, "%s/May 2026", dbs);
    CHECK(dbfolders_valid(d) == 1);
    CHECK(dbfolders_path(tmpl, "May 2026", d, sizeof d) == 1);
    CHECK(strstr(d, "Databases/May 2026") != NULL);
    CHECK(dbfolders_path(tmpl, "No staff", d, sizeof d) == 0);
    CHECK(dbfolders_path(tmpl, "../Databases/May 2026", d, sizeof d) == 0);   /* no path parts in a name */
    CHECK(dbfolders_path(tmpl, "gone", d, sizeof d) == 0);

    /* No Databases folder: an empty list. */
    char none[] = "/tmp/dbfolders_none_XXXXXX";
    if (!mkdtemp(none)) return 2;
    CHECK(dbfolders_list(none, names, DBFOLDER_MAX) == 0);
    rmdir(none);

    /* The button text: code page 1252, "e with diaeresis" is the byte 0xeb. */
    char text[64];
    dbfolders_label("Zo\xc3\xab Update", text, sizeof text);
    CHECK(!strcmp(text, "Zo\xeb Update"));
    dbfolders_label("A very long folder name that goes on and on", text, 20);
    CHECK(strlen(text) == 19);

    /* Remove the test folders: only the names made above. */
    static const char *const sets[] = {"May 2026", "Zo\xc3\xab Update", "Upper", "No staff"};
    for (size_t s = 0; s < 4; s++) {
        snprintf(d, sizeof d, "%s/%s", dbs, sets[s]);
        for (size_t i = 0; i < sizeof files / sizeof *files; i++) {
            char f[2200], n2[64];
            snprintf(n2, sizeof n2, "%s", files[i]);
            if (s == 2) for (char *c = n2; *c; c++) if (*c >= 'a' && *c <= 'z') *c -= 32;
            snprintf(f, sizeof f, "%s/%s", d, n2);
            unlink(f);
        }
        rmdir(d);
    }
    snprintf(d, sizeof d, "%s/Notes/readme.txt", dbs);
    unlink(d);
    snprintf(d, sizeof d, "%s/Notes", dbs);
    rmdir(d);
    snprintf(d, sizeof d, "%s/Empty", dbs);
    rmdir(d);
    snprintf(d, sizeof d, "%s/stray.dat", dbs);
    unlink(d);
    rmdir(dbs);
    rmdir(tmpl);
    if (failures) { printf("%d failures\n", failures); return 1; }
    printf("dbfolders: ok\n");
    return 0;
}
