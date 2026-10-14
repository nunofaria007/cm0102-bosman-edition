#include "date.h"
#include "scrman_input.h"

/* The day of the year before the 1st of each month: 0x9b8448, 0x9b8460. */
static const s16 g_month_start[12] = {
    -1, 30, 58, 89, 119, 150, 180, 211, 242, 272, 303, 333
};
static const s16 g_month_start_leap[12] = {
    -1, 30, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334
};

/* The leap test that each function of Date.cpp repeats inline. */
static bool is_leap(u16 year)
{
    if (year % 2 == 1 || year % 4 != 0) return false;
    return year % 100 != 0 || year % 400 == 0;
}

/* The body of FUN_00549f70 and FUN_0054a130, which differ in their error lines. */
static void date_fill(cm_date_t *d, s16 day, s8 month, u16 year, s16 weekday)
{
    if (day < 1 || day > 31) {
        /* The original shows an error box here (Date.cpp 0x34 or 0x72). */
        g_left_held = 0;
        day = 1;
    }
    if (month < 0 || month > 11) {
        /* The original shows an error box here (Date.cpp 0x3b or 0x79). */
        g_left_held = 0;
        month = 0;
    }
    d->year = year;
    if (is_leap(year)) {
        d->leap = 1;
        d->day = (s16)(g_month_start_leap[month] + day);
    } else {
        d->leap = 0;
        d->day = (s16)(g_month_start[month] + day);
    }
    if (weekday != -1) date_to_weekday(d, weekday, d);
}

cm_date_t *date_init(cm_date_t *d, s16 day, s8 month, u16 year, s16 weekday)
{
    date_fill(d, day, month, year, weekday);
    return d;
}

void date_set(cm_date_t *d, s16 day, s8 month, u16 year, s16 weekday)
{
    date_fill(d, day, month, year, weekday);
}

void date_default(cm_date_t *d)
{
    d->day = 40;
    d->year = 1972;
    d->leap = 1;
}

cm_date_t date_copy(cm_date_t *d, const cm_date_t *src)
{
    *d = *src;
    return *d;
}

/*
 * FUN_0054a3f0. The days from 1 Jan 1600 modulo 7, where 1 Jan 1600 is a
 * Saturday. Before 1600 the remainder is negative and comes back as it is,
 * -6 to -1.
 */
s16 date_weekday(const cm_date_t *d)
{
    s32 y = (s32)d->year - 1600;
    s32 n = y * 365 + y / 400 - y / 100 + y / 4 + d->day;
    s16 r;

    if (d->leap) n--;
    r = (s16)(n % 7);
    if (r > 0) return (s16)(r - 1);
    return r == 0 ? DATE_SUNDAY : r;
}

/*
 * FUN_0054a2d0. The original computes the weekday inline, as date_weekday.
 * A step past three days goes the other way round the week.
 */
void date_to_weekday(const cm_date_t *d, s16 weekday, cm_date_t *out)
{
    s16 diff = date_weekday(d);

    *out = *d;
    if (diff == weekday) return;
    diff = (s16)(diff - weekday);
    if (diff < 0) {
        if (diff < -3) diff = (s16)(diff + 7);
    } else if (diff > 3) {
        diff = (s16)(diff - 7);
    }
    *out = date_sub_days(out, diff);
}

/* The year loop of FUN_0054c5b0 and FUN_0054c770. */
static cm_date_t move_forward(u16 year, s16 day)
{
    cm_date_t r;

    for (;;) {
        if (is_leap(year)) {
            if (day <= 365) break;
            day = (s16)(day - 366);
        } else {
            if (day <= 364) break;
            day = (s16)(day - 365);
        }
        year++;
    }
    r.day = day;
    r.year = year;
    r.leap = is_leap(year);
    return r;
}

/* The year loop of FUN_0054c8e0 and FUN_0054cab0. */
static cm_date_t move_back(u16 year, s16 day)
{
    cm_date_t r;

    while (day < 0) {
        u16 before = (u16)(year - 1);

        day = (s16)(day + (is_leap(before) ? 366 : 365));
        year = before;
    }
    r.day = day;
    r.year = year;
    r.leap = is_leap(year);
    return r;
}

/*
 * The original keeps the result in a function static and copies it out.
 * With -0x8000 days, it and date_sub_days call each other without end.
 */
cm_date_t date_add_days(const cm_date_t *d, s16 days)
{
    if (days < 0) return date_sub_days(d, (s16)-days);
    return move_forward(d->year, (s16)(d->day + days));
}

cm_date_t date_sub_days(const cm_date_t *d, s16 days)
{
    if (days < 0) return date_add_days(d, (s16)-days);
    return move_back(d->year, (s16)(d->day - days));
}

cm_date_t date_add_weeks(const cm_date_t *d, u8 weeks)
{
    return move_forward(d->year, (s16)(d->day + weeks * 7));
}

cm_date_t date_sub_weeks(const cm_date_t *d, u8 weeks)
{
    return move_back(d->year, (s16)(d->day - weeks * 7));
}
