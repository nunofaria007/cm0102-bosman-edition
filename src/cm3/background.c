#include "background.h"
#include "screen_stack.h"
#include "utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>

#define LINE_MAX_CHARS 0x7ff
#define NAME_MAX_CHARS 259

pics_section_t g_pics[PICS_SECTIONS] = {
    {"nations", 0, NULL},   {"clubs", 0, NULL},   {"stadiums", 0, NULL},
    {"comps", 0, NULL},     {"awards", 0, NULL},  {"staff", 0, NULL},
    {"training", 0, NULL},  {"boardroom", 0, NULL}, {"trophy", 0, NULL},
    {"transfer", 0, NULL},  {"si", 0, NULL},
};

static bool g_pics_loaded;

/*
 * fgetc on a file that the original opens in text mode: the MSVC library
 * turns CR LF into LF and ends the file at the byte 0x1a.
 */
static s32 read_char(FILE *f, bool *eof)
{
    s32 c;

    if (*eof) return EOF;
    c = fgetc(f);
    if (c == '\r') {
        s32 next = fgetc(f);

        if (next == '\n') return '\n';
        if (next != EOF) ungetc(next, f);
    }
    if (c == EOF || c == 0x1a) {
        *eof = true;
        return EOF;
    }
    return c;
}

/*
 * FUN_0090cba0. One line with text. Tab, CR and space at the line start are
 * skipped, and empty lines too. "#" drops the rest of the line, and the line
 * goes on with the next one. The byte 0xff ends the file, as EOF does.
 */
static bool read_line(FILE *f, char *line, bool *eof)
{
    s32 n = 0;
    s8 c = (s8)read_char(f, eof);

    while (c != -1) {
        switch (c) {
        case '\t':
        case '\r':
        case ' ':
            if (n && n < LINE_MAX_CHARS) line[n++] = c;
            break;
        case '\n':
            if (n) {
                line[n] = '\0';
                return true;
            }
            break;
        case '#':
            while (c != '\n' && c != -1) c = (s8)read_char(f, eof);
            break;
        default:
            if (n < LINE_MAX_CHARS) line[n++] = c;
            break;
        }
        c = (s8)read_char(f, eof);
    }
    line[n] = '\0';
    return n != 0;
}

static bool opens(const char *name)
{
    FILE *f = fopen(utils_path("CM3_PICTURES", name), "rb");

    if (!f) return false;
    fclose(f);
    return true;
}

/*
 * FUN_0041b8c0. Returns false when it runs out of memory. A name that no
 * file has is dropped, which is not an error. `name` becomes "_name" when
 * the second round of tries runs, as in the original.
 */
static bool add_name(pics_entry_t *entry, char *name, bool second)
{
    char form[NAME_MAX_CHARS + 16];

    for (;;) {
        snprintf(form, sizeof(form), "%s", name);
        if (opens(form)) break;
        snprintf(form, sizeof(form), "stadium\\%s", name);
        if (opens(form)) break;
        snprintf(form, sizeof(form), "old\\%s", name);
        if (opens(form)) break;
        if (name[0] == '_') return true;
        snprintf(form, sizeof(form), "_%s", name);
        snprintf(name, NAME_MAX_CHARS + 1, "%s", form);
    }

    char ***list = second ? &entry->names2 : &entry->names;
    s16 *count = second ? &entry->count2 : &entry->count;
    char **grown = realloc(*list, sizeof(char *) * (size_t)(*count + 1));

    if (!grown) return false;
    *list = grown;
    grown[*count] = strdup(form);
    return grown[(*count)++] != NULL;
}

/* The names after the key: "," and "}" end a name, "{" ends one for names2. */
static bool read_names(pics_entry_t *entry, const char *p, char end)
{
    char name[NAME_MAX_CHARS + 1];
    s32 length = 0;
    bool second = false;
    char c = end;

    while (c) {
        c = *p++;
        switch (c) {
        case '{':
            second = true;
            /* fall through */
        case '\0':
        case ',':
        case '}':
            name[length] = '\0';
            if (name[0] == '\0') break;
            if (!add_name(entry, name, second)) return false;
            second = false;
            length = 0;
            break;
        case ' ':
            break;
        default:
            if (length < NAME_MAX_CHARS) name[length++] = c;
            break;
        }
    }
    return true;
}

/* A "key: names" line in section `s`. Returns false when memory runs out. */
static bool read_entry(pics_section_t *s, const char *line)
{
    char key[LINE_MAX_CHARS + 1];
    s32 n = 0;
    char c;

    do {
        c = line[n];
        key[n++] = c;
    } while (c && c != ':');
    key[n - 1] = '\0';

    pics_entry_t *entries = realloc(s->entries,
                                    sizeof(*entries) * (size_t)(s->count + 1));

    if (!entries) return false;
    s->entries = entries;

    pics_entry_t *entry = &entries[s->count];

    entry->key = strdup(key);
    if (!entry->key) return false;
    s->count++;
    entry->count = entry->count2 = 0;
    entry->names = entry->names2 = NULL;
    if (!read_names(entry, line + n, c)) return false;
    if (entry->count == 0 && entry->count2 == 0) {
        free(entry->key);
        entry->key = NULL;
        s->count--;
    }
    return true;
}

void pics_load(void)
{
    static char line[LINE_MAX_CHARS + 1];
    FILE *f = fopen(utils_path("CM3_PICTURES", "pics.cfg"), "rb");
    s32 section = -1;
    bool eof = false;

    g_pics_loaded = true;
    if (!f) return;
    while (!eof && read_line(f, line, &eof)) {
        if (line[0] == '[') {
            char *name = line + 1;

            name[strcspn(name, "]")] = '\0';
            section = -1;
            for (s32 i = 0; i < PICS_SECTIONS; i++) {
                if (!strcasecmp(g_pics[i].name, name)) {
                    section = i;
                    break;
                }
            }
            continue;
        }
        /* The original shows an error box here in a debug run. */
        if (section == -1) continue;
        if (!read_entry(&g_pics[section], line)) break;
    }
    fclose(f);
}

void pics_free(void)
{
    for (s32 i = 0; i < PICS_SECTIONS; i++) {
        pics_section_t *s = &g_pics[i];

        for (s32 k = 0; k < s->count; k++) {
            pics_entry_t *e = &s->entries[k];

            for (s32 n = 0; n < e->count; n++) free(e->names[n]);
            for (s32 n = 0; n < e->count2; n++) free(e->names2[n]);
            free(e->names);
            free(e->names2);
            free(e->key);
        }
        free(s->entries);
        s->entries = NULL;
        s->count = 0;
    }
    g_pics_loaded = false;
}

/* FUN_0090cfc0 for a count of 0 to 0x7fff. */
static s16 random_below(s32 n)
{
    return n > 0 ? (s16)(rand() % n) : 0;
}

void pick_background(void)
{
    screen_stack_t *st = &g_screen_stack;
    const char *title = screen_stack_title(st);
    s16 section, tries = 0;

    if (!g_pics_loaded) pics_load();
    if (title[0] != '\\' && title[0] != '\0') return;
    section = random_below(6);
    do {
        const pics_section_t *s = &g_pics[section];

        /* Section 2 holds the stadiums. */
        if (s->entries && section != 2) {
            const pics_entry_t *e = &s->entries[random_below(s->count)];

            if (e->names) {
                const char *name = e->names[random_below(e->count)];

                if (name) {
                    screen_stack_set_title(st,
                                           utils_path("CM3_PICTURES", name));
                    return;
                }
            }
        }
        tries++;
        if (++section >= 6) section = 0;
    } while (tries < 6);
    screen_stack_set_title(st, ":");
}
