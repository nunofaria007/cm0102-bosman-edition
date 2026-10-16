/* The reader of the settings file (options.h). */
#include "options.h"
#include "../host.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef enum { KIND_SWITCH, KIND_TEXT, KIND_ENUM } Kind;

typedef struct {
    const char *name;
    Kind kind;
    const char *def;           /* the default, as written in the file */
    const char *values;        /* KIND_ENUM: the values, separated by "|" */
    const char *comment;       /* the one comment line before the option in the file */
} OptionDef;

static const OptionDef defs[OPT_COUNT] = {
    [OPT_DISPLAY] = {"display", KIND_ENUM, "widescreen", "widescreen|classic|hd",
        "display = widescreen draws the game at 1280x800. display = classic draws it at 800x600. display = hd draws it at 1920x1080."},
    [OPT_SPACE_TO_CONTINUE] = {"space_to_continue", KIND_SWITCH, "on", NULL,
        "space_to_continue = on lets the space bar press Continue Game."},
    [OPT_SHORTCUTS] = {"shortcuts", KIND_SWITCH, "on", NULL,
        "shortcuts = on adds keys for Back, for closing a popup and for full screen."},
    [OPT_SHARP_PICTURE] = {"sharp_picture", KIND_SWITCH, "on", NULL,
        "sharp_picture = on scales the picture to the window in whole pixels, so that text stays crisp."},
    [OPT_SAVE_FOLDER] = {"save_folder", KIND_TEXT, "", NULL,
        "save_folder = the folder for saved games, for example in iCloud Drive. Empty uses the normal folder."},
    [OPT_SOUND] = {"sound", KIND_SWITCH, "on", NULL,
        "sound = off mutes the game. With sound = on, the game's own Sound setting still applies."},
    [OPT_CLICKABLE_NAMES] = {"clickable_names", KIND_SWITCH, "on", NULL,
        "clickable_names = on lets you click a player or club name to open its profile."},
    [OPT_ATTRIBUTE_COLOURS] = {"attribute_colours", KIND_SWITCH, "on", NULL,
        "attribute_colours = on colours attribute numbers by value."},
    [OPT_FASTER_DAYS] = {"faster_days", KIND_SWITCH, "on", NULL,
        "faster_days = on makes days pass faster while you are on holiday."},
    [OPT_MATCH_COLUMNS] = {"match_columns", KIND_SWITCH, "on", NULL,
        "match_columns = on shows position, condition and match rating together on the match screens (widescreen only)."},
    [OPT_REMEMBER_WINDOW] = {"remember_window", KIND_SWITCH, "on", NULL,
        "remember_window = on opens the window with the size, place and full screen state that it had at the last quit."},
    [OPT_SAVE_BACKUPS] = {"save_backups", KIND_SWITCH, "on", NULL,
        "save_backups = on keeps the last three copies of each save in the folder backups next to the saves."},
    [OPT_CONDITION_COLOURS] = {"condition_colours", KIND_SWITCH, "on", NULL,
        "condition_colours = on colours condition percentages and morale words: red, amber or green."},
    [OPT_DATABASE] = {"database", KIND_ENUM, "3.9.68", "3.9.68|3.9.60|3.9.65",
        "database = the data set of a new game: 3.9.68 (2002/03), 3.9.60 (2001/02), 3.9.65 (2001/02, December update) or folder:<name> for a folder in Databases."},
    [OPT_SQUAD_VIEWS] = {"squad_views", KIND_SWITCH, "on", NULL,
        "squad_views = on adds Team views (best XI, squad depth, compare players) to the Assistant drop-down of a squad screen."},
};

#define TEXT_MAX 1100
static char text_value[OPT_COUNT][TEXT_MAX];   /* the current text of each option */
static int present[OPT_COUNT];                 /* the file has a line for the option */
static int loaded;
static char file_path[TEXT_MAX];               /* the settings file of options_load */

static void lower(char *s)
{
    for (; *s; s++) *s = (char)tolower((unsigned char)*s);
}

/* 1 or 0 for a switch word, -1 for any other word. */
static int parse_switch(const char *v)
{
    static const char *const yes[] = {"on", "yes", "true"}, *const no[] = {"off", "no", "false"};
    for (int i = 0; i < 3; i++) {
        if (!strcmp(v, yes[i])) return 1;
        if (!strcmp(v, no[i])) return 0;
    }
    return -1;
}

/* The index of word v in the list "a|b|c", or -1. */
static int parse_enum(const char *list, const char *v)
{
    size_t n = strlen(v);
    int index = 0;
    for (const char *p = list; ; index++) {
        const char *end = strchr(p, '|');
        size_t len = end ? (size_t)(end - p) : strlen(p);
        if (len == n && !strncmp(p, v, n)) return index;
        if (!end) return -1;
        p = end + 1;
    }
}

static void reset(void)
{
    for (int id = 0; id < OPT_COUNT; id++) {
        snprintf(text_value[id], TEXT_MAX, "%s", defs[id].def);
        present[id] = 0;
    }
    loaded = 1;
}

/* A custom data set: "folder:" and a name. The name keeps its case. */
static int is_custom_database(int id, const char *v)
{
    return id == OPT_DATABASE && !strncmp(v, "folder:", 7) && v[7];
}

/* Sets option id from the word in the file. A word that does not fit keeps the default. */
static void set_value(const char *path, int id, const char *value)
{
    const OptionDef *d = &defs[id];
    char word[TEXT_MAX];
    snprintf(word, sizeof word, "%s", value);
    if (d->kind == KIND_TEXT) {
        snprintf(text_value[id], TEXT_MAX, "%s", word);
        return;
    }
    if (is_custom_database(id, word)) {
        snprintf(text_value[id], TEXT_MAX, "%s", word);
        return;
    }
    lower(word);
    int ok = d->kind == KIND_SWITCH ? parse_switch(word) >= 0 : parse_enum(d->values, word) >= 0;
    if (ok) {
        snprintf(text_value[id], TEXT_MAX, "%s", word);
    } else {
        snprintf(text_value[id], TEXT_MAX, "%s", d->def);
        fprintf(stderr, "cm0102_recomp: %s: unknown value \"%s\" for %s, so the default %s is used.\n", path,
                value, d->name, d->def);
    }
}

/* Reads one line. Sets present[] and the value for a known option. Other lines have no effect. */
static void read_line(const char *path, char *line)
{
    char *p = line;
    while (isspace((unsigned char)*p)) p++;
    if (*p == '#' || !*p) return;
    char key[64];
    size_t n = 0;
    while (isalnum((unsigned char)*p) || *p == '_') {
        if (n < sizeof key - 1) key[n++] = (char)tolower((unsigned char)*p);
        p++;
    }
    key[n] = 0;
    while (*p == ' ' || *p == '\t') p++;
    if (!n || *p != '=') return;
    p++;
    while (isspace((unsigned char)*p)) p++;
    char *end = p + strlen(p);
    while (end > p && isspace((unsigned char)end[-1])) end--;
    *end = 0;
    for (int id = 0; id < OPT_COUNT; id++) {
        if (strcmp(key, defs[id].name)) continue;
        present[id] = 1;
        set_value(path, id, p);
        return;
    }
}

void options_load(const char *path)
{
    reset();
    snprintf(file_path, sizeof file_path, "%s", path);
    int missing_file = 0, ends_with_newline = 1;
    FILE *f = host_fopen(path, "r");
    if (!f) {
        missing_file = 1;
    } else {
        char line[TEXT_MAX + 128];
        int c, len = 0, too_long = 0;
        /* Read by character, so that a long line is cut and not read as two. */
        while ((c = fgetc(f)) != EOF) {
            ends_with_newline = c == '\n';
            if (c == '\n') {
                line[len] = 0;
                if (!too_long) read_line(path, line);
                len = 0;
                too_long = 0;
            } else if (len < (int)sizeof line - 1) {
                line[len++] = (char)c;
            } else {
                too_long = 1;
            }
        }
        if (len && !too_long) {
            line[len] = 0;
            read_line(path, line);
        }
        fclose(f);
    }

    int missing = 0;
    for (int id = 0; id < OPT_COUNT; id++) missing += !present[id];
    if (!missing) return;

    /* A new file gets a header. A file of the player only gets the lines that it lacks, at the end. */
    f = host_fopen(path, missing_file ? "w" : "a");
    if (!f) return;
    if (missing_file) {
        fputs("# CM 01/02: Bosman Edition settings.\n"
              "# Each option has one line: name = value. A switch takes on or off. Yes, no, true and false work too.\n",
              f);
    } else if (!ends_with_newline) {
        fputc('\n', f);
    }
    for (int id = 0; id < OPT_COUNT; id++) {
        if (present[id]) continue;
        fprintf(f, "# %s\n%s = %s\n", defs[id].comment, defs[id].name, defs[id].def);
    }
    fclose(f);
}

int options_bool(OptionId id)
{
    if (!loaded) reset();
    return id < OPT_COUNT && defs[id].kind == KIND_SWITCH && parse_switch(text_value[id]) == 1;
}

const char *options_string(OptionId id)
{
    if (!loaded) reset();
    return id < OPT_COUNT ? text_value[id] : "";
}

int options_enum(OptionId id)
{
    if (!loaded) reset();
    if (id >= OPT_COUNT || defs[id].kind != KIND_ENUM) return 0;
    if (is_custom_database((int)id, text_value[id])) return DATABASE_CUSTOM;
    return parse_enum(defs[id].values, text_value[id]);
}

/* The option name of a line, in lower case, or "" for a comment or another line. */
static void option_key(const char *line, char *key, size_t room)
{
    const char *p = line;
    size_t n = 0;
    while (isspace((unsigned char)*p)) p++;
    while (isalnum((unsigned char)*p) || *p == '_') {
        if (n < room - 1) key[n++] = (char)tolower((unsigned char)*p);
        p++;
    }
    while (*p == ' ' || *p == '\t') p++;
    key[*p == '=' ? n : 0] = 0;
}

int options_set_enum(OptionId id, const char *value)
{
    if (!loaded) reset();
    if (id >= OPT_COUNT || defs[id].kind != KIND_ENUM) return 0;
    if (!is_custom_database((int)id, value) && parse_enum(defs[id].values, value) < 0) return 0;
    snprintf(text_value[id], TEXT_MAX, "%s", value);
    if (!file_path[0]) return 1;
    /* Read the whole file, write it again with the new value on the option's lines to a temporary file
     * next to it, then move that over the file. A failed write leaves the player's file as it was. */
    FILE *f = host_fopen(file_path, "r");
    if (!f) return 1;
    size_t cap = 65536, n = 0;
    char *text = malloc(cap + 1);
    while (text) {
        size_t got = fread(text + n, 1, cap - n, f);
        n += got;
        if (n < cap) break;
        char *bigger = realloc(text, cap * 2 + 1);
        if (!bigger) { free(text); text = NULL; break; }
        text = bigger;
        cap *= 2;
    }
    int read_ok = text && !ferror(f);
    fclose(f);
    if (!read_ok) {
        free(text);
        fprintf(stderr, "cm0102_recomp: cannot read %s, so %s is not saved\n", file_path, defs[id].name);
        return 1;
    }
    text[n] = 0;
    char tmp_path[sizeof file_path + 8];
    snprintf(tmp_path, sizeof tmp_path, "%s.tmp", file_path);
    f = host_fopen(tmp_path, "w");
    if (!f) {
        free(text);
        fprintf(stderr, "cm0102_recomp: cannot write %s, so %s is not saved\n", tmp_path, defs[id].name);
        return 1;
    }
    int found = 0;
    for (char *line = text; *line;) {
        char *end = strchr(line, '\n');
        size_t len = end ? (size_t)(end - line) + 1 : strlen(line);
        char key[64];
        option_key(line, key, sizeof key);
        if (!strcmp(key, defs[id].name)) {
            fprintf(f, "%s = %s%s", defs[id].name, value, end ? (len > 1 && end[-1] == '\r' ? "\r\n" : "\n") : "");
            found = 1;
        } else {
            fwrite(line, 1, len, f);
        }
        line += len;
    }
    if (!found) fprintf(f, "%s# %s\n%s = %s\n", n && text[n - 1] != '\n' ? "\n" : "", defs[id].comment,
                        defs[id].name, value);
    free(text);
    int write_ok = !ferror(f);
    if (fclose(f) != 0) write_ok = 0;
    /* Windows does not rename over a file: remove the old one when the first try fails. */
    if (write_ok && host_rename(tmp_path, file_path) != 0 && (host_unlink(file_path) != 0 || host_rename(tmp_path, file_path) != 0))
        write_ok = 0;
    if (!write_ok) {
        host_unlink(tmp_path);
        fprintf(stderr, "cm0102_recomp: cannot write %s, so %s is not saved\n", file_path, defs[id].name);
    }
    return 1;
}

const char *options_name(OptionId id)
{
    return id < OPT_COUNT ? defs[id].name : "";
}
