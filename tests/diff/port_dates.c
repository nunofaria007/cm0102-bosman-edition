/*
 * Commands of the port driver for the dates (tests/diff/cases_dates.py) and
 * the random numbers (tests/diff/cases_random.py). A date is sent and
 * printed as its day, year and leap.
 */
#include "port_dates.h"
#include "date.h"
#include "port_helpers.h"
#include "random.h"
#include "scrman_input.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static u32 next_u32(const char **args)
{
    char *end;
    u32 value = (u32)strtoul(*args, &end, 10);

    *args = end;
    return value;
}

static cm_date_t read_date(const char **args)
{
    cm_date_t d;

    d.day = (s16)next_long(args);
    d.year = (u16)next_long(args);
    d.leap = next_u32(args);
    return d;
}

static void print_date(const cm_date_t *d, const char *end)
{
    printf("%d %u %u%s", d->day, d->year, d->leap, end);
}

/* Reads the first word of the arguments into `word` and moves past it. */
static void next_word(const char **args, char *word)
{
    int used = 0;

    word[0] = '\0';
    sscanf(*args, "%15s%n", word, &used);
    *args += used;
}

static bool date_command(const char *cmd, const char *args)
{
    char word[16];

    if (!strcmp(cmd, "dt_init")) {
        /* "dt_init init|set day month year weekday" */
        cm_date_t d;

        next_word(&args, word);
        long day = next_long(&args), month = next_long(&args);
        long year = next_long(&args), weekday = next_long(&args);
        g_left_held = 1;
        memset(&d, 0xa5, sizeof(d));
        if (!strcmp(word, "init")) {
            if (date_init(&d, (s16)day, (s8)month, (u16)year, (s16)weekday) != &d) {
                printf("bad\n");
                return true;
            }
        } else {
            date_set(&d, (s16)day, (s8)month, (u16)year, (s16)weekday);
        }
        print_date(&d, "");
        printf(" %d\n", g_left_held);
    } else if (!strcmp(cmd, "dt_default")) {
        cm_date_t d;

        memset(&d, 0xff, sizeof(d));
        date_default(&d);
        print_date(&d, "\n");
    } else if (!strcmp(cmd, "dt_weekday")) {
        cm_date_t d = read_date(&args);

        printf("%d\n", date_weekday(&d));
    } else if (!strcmp(cmd, "dt_to_weekday")) {
        cm_date_t d = read_date(&args);
        long weekday = next_long(&args);

        date_to_weekday(&d, (s16)weekday, &d);
        print_date(&d, "\n");
    } else if (!strcmp(cmd, "dt_copy")) {
        cm_date_t src = read_date(&args), copy, out;

        memset(&copy, 0x5a, sizeof(copy));
        out = date_copy(&copy, &src);
        print_date(&copy, " ");
        print_date(&out, "\n");
    } else if (!strcmp(cmd, "dt_op")) {
        /* "dt_op add|sub|addw|subw day year leap count" */
        cm_date_t d, r;

        next_word(&args, word);
        d = read_date(&args);
        long count = next_long(&args);
        if (!strcmp(word, "add")) {
            r = date_add_days(&d, (s16)count);
        } else if (!strcmp(word, "sub")) {
            r = date_sub_days(&d, (s16)count);
        } else if (!strcmp(word, "addw")) {
            r = date_add_weeks(&d, (u8)count);
        } else {
            r = date_sub_weeks(&d, (u8)count);
        }
        print_date(&r, "\n");
    } else {
        return false;
    }
    return true;
}

static bool random_command(const char *cmd, const char *args)
{
    if (!strcmp(cmd, "rn_load")) {
        char path[1024] = "", error[512] = "";

        sscanf(args, "%1023s", path);
        if (random_load_table(path, error, sizeof(error))) {
            printf("1\n");
        } else {
            printf("0 %s\n", error);
        }
    } else if (!strcmp(cmd, "rn_table")) {
        /* "rn_table a b c": three numbers of the table. */
        for (int i = 0; i < 3; i++) {
            long index = next_long(&args);

            if (index < 0 || index >= RANDOM_TABLE_SIZE) index = 0;
            printf("%d%s", g_random_table[index], i < 2 ? " " : "\n");
        }
    } else if (!strcmp(cmd, "rn_state")) {
        /* "rn_state holdrand index end offset" */
        g_holdrand = next_u32(&args);
        g_random_index = (s32)next_long(&args);
        g_random_end = (s32)next_long(&args);
        g_random_offset = (s32)next_long(&args);
    } else if (!strcmp(cmd, "rn_get")) {
        printf("%u %d %d %d\n", g_holdrand, g_random_index, g_random_end,
               g_random_offset);
    } else if (!strcmp(cmd, "rn_srand")) {
        /* "rn_srand seed count": the first numbers of rand after the seed. */
        u32 seed = next_u32(&args);
        long count = next_long(&args);

        cm_srand(seed);
        for (long i = 0; i < count; i++) {
            printf("%d%s", cm_rand(), i + 1 < count ? " " : "\n");
        }
    } else if (!strcmp(cmd, "rn_seed")) {
        random_seed((s32)next_long(&args));
        printf("%s\n", g_random_seed_text);
    } else if (!strcmp(cmd, "rn_run")) {
        /* "rn_run number count ...": one random number for each count. */
        long number = next_long(&args);

        for (long i = 0; i < number; i++) {
            printf("%d%s", random_number((s32)next_long(&args)),
                   i + 1 < number ? " " : "");
        }
        printf("\n");
    } else {
        return false;
    }
    return true;
}

bool dates_command(const char *cmd, const char *args)
{
    return date_command(cmd, args) || random_command(cmd, args);
}
