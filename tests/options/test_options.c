/* Test of the settings file reader (src/recomp_rt/win/options.c).
 * Usage: options_test   (it makes and removes its own temporary folder) */
#include "options.h"
#include "../../src/recomp_rt/host.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

static int failures;
static char dir[1024], path[1100];

#define CHECK(cond) do { if (!(cond)) { printf("FAIL line %d: %s\n", __LINE__, #cond); failures++; } } while (0)

static void put(const char *text)
{
    FILE *f = host_fopen(path, "w");
    if (!f) { printf("cannot write %s\n", path); exit(2); }
    fputs(text, f);
    fclose(f);
}

static char *get(void)
{
    static char buf[16384];
    FILE *f = host_fopen(path, "r");
    size_t n = f ? fread(buf, 1, sizeof buf - 1, f) : 0;
    if (f) fclose(f);
    buf[n] = 0;
    return buf;
}

static int count(const char *hay, const char *needle)
{
    int n = 0;
    for (const char *p = hay; (p = strstr(p, needle)); p += strlen(needle)) n++;
    return n;
}

/* The text has exactly one line "name = " for the option (the line may start the text). */
static int has_one_line(const char *text, OptionId id)
{
    char key[80];
    snprintf(key, sizeof key, "\n%s = ", options_name(id));
    return count(text, key) + (!strncmp(text, key + 1, strlen(key) - 1)) == 1;
}

/* The text has exactly one comment line for the option. */
static int has_one_comment(const char *text, OptionId id)
{
    char key[80];
    snprintf(key, sizeof key, "# %s = ", options_name(id));
    return count(text, key) == 1;
}

static void check_defaults(void)
{
    CHECK(options_enum(OPT_DISPLAY) == DISPLAY_WIDESCREEN);
    CHECK(options_bool(OPT_SPACE_TO_CONTINUE) == 1);
    CHECK(options_bool(OPT_SHORTCUTS) == 1);
    CHECK(options_bool(OPT_SHARP_PICTURE) == 1);
    CHECK(!strcmp(options_string(OPT_SAVE_FOLDER), ""));
    CHECK(options_bool(OPT_SOUND) == 1);
    CHECK(options_bool(OPT_ASSISTANT_SELECTION) == 0);
    CHECK(options_bool(OPT_CLICKABLE_NAMES) == 1);
    CHECK(options_bool(OPT_ATTRIBUTE_COLOURS) == 1);
    CHECK(options_bool(OPT_FASTER_DAYS) == 1);
    CHECK(options_bool(OPT_MATCH_COLUMNS) == 1);
}

int main(void)
{
    snprintf(dir, sizeof dir, "%s/options_test_XXXXXX", getenv("TMPDIR") ? getenv("TMPDIR") : "/tmp");
    if (!mkdtemp(dir)) { perror("mkdtemp"); return 2; }
    snprintf(path, sizeof path, "%s/settings.txt", dir);

    /* 1. A missing file is written with every option at its default. */
    options_load(path);
    check_defaults();
    for (int id = 0; id < OPT_COUNT; id++) {
        CHECK(has_one_line(get(), (OptionId)id));
        CHECK(has_one_comment(get(), (OptionId)id));
    }
    CHECK(strstr(get(), "\nassistant_selection = off\n") != NULL);
    CHECK(strstr(get(), "\ndisplay = widescreen\n") != NULL);
    CHECK(strstr(get(), "\nsave_folder = \n") != NULL);
    static char first[16384];
    snprintf(first, sizeof first, "%s", get());
    options_load(path);                         /* a second start changes nothing */
    CHECK(!strcmp(first, get()));
    CHECK(!strcmp(options_string(OPT_SAVE_FOLDER), ""));

    /* 2. Unknown lines and the order of the player's lines stay. */
    const char *mine = "# my notes\nzebra = 1\n\nDisplay = Classic\nSOUND = No\n"
                       "save_folder =  /Users/me/My Saves \nfuture_option=yes\n";
    put(mine);
    options_load(path);
    const char *t = get();
    CHECK(options_enum(OPT_DISPLAY) == DISPLAY_CLASSIC);
    CHECK(options_bool(OPT_SOUND) == 0);
    CHECK(!strcmp(options_string(OPT_SAVE_FOLDER), "/Users/me/My Saves"));
    CHECK(options_bool(OPT_SHORTCUTS) == 1);
    CHECK(!strncmp(t, mine, strlen(mine)));
    CHECK(count(t, "zebra = 1") == 1 && count(t, "future_option=yes") == 1);
    CHECK(count(t, "\nsound = ") == 0 && count(t, "\ndisplay = ") == 0 && count(t, "\nsave_folder = ") == 1);
    CHECK(strstr(t, "\nshortcuts = on\n") != NULL);

    /* 3. An old file with only display = classic keeps classic and gains the new lines. */
    const char *old = "# CM 01/02: Bosman Edition settings.\n"
                      "# display = widescreen draws the game at 1280x800. display = classic draws it at 800x600.\n"
                      "display = classic\n";
    put(old);
    options_load(path);
    t = get();
    CHECK(options_enum(OPT_DISPLAY) == DISPLAY_CLASSIC);
    CHECK(options_bool(OPT_FASTER_DAYS) == 1 && options_bool(OPT_ASSISTANT_SELECTION) == 0);
    CHECK(!strncmp(t, old, strlen(old)));
    CHECK(count(t, "\ndisplay = classic\n") == 1 && count(t, "\ndisplay = ") == 1);
    for (int id = 0; id < OPT_COUNT; id++) {
        if (id == OPT_DISPLAY) continue;
        CHECK(has_one_line(t, (OptionId)id));
        CHECK(has_one_comment(t, (OptionId)id));
    }
    static char again[16384];
    snprintf(again, sizeof again, "%s", t);
    options_load(path);                         /* the added lines are not added twice */
    CHECK(!strcmp(again, get()));

    /* 4. A last line without a newline gets one before the added lines. */
    put("display = classic");
    options_load(path);
    CHECK(!strncmp(get(), "display = classic\n# ", 19));

    /* 5. Words for the switches, and a bad value keeps the default. */
    put("space_to_continue = FALSE\nshortcuts = yes\nsharp_picture = off\nsound = true\n"
        "clickable_names = maybe\ndisplay = tiny\nassistant_selection = on\r\nfaster_days = off\r\n");
    options_load(path);
    CHECK(options_bool(OPT_SPACE_TO_CONTINUE) == 0);
    CHECK(options_bool(OPT_SHORTCUTS) == 1);
    CHECK(options_bool(OPT_SHARP_PICTURE) == 0);
    CHECK(options_bool(OPT_SOUND) == 1);
    CHECK(options_bool(OPT_CLICKABLE_NAMES) == 1);
    CHECK(options_enum(OPT_DISPLAY) == DISPLAY_WIDESCREEN);
    CHECK(options_bool(OPT_ASSISTANT_SELECTION) == 1);
    CHECK(options_bool(OPT_FASTER_DAYS) == 0);

    host_unlink(path);
    rmdir(dir);
    if (failures) printf("options_test: %d failures\n", failures);
    else printf("options_test: ok\n");
    return failures ? 1 : 0;
}
