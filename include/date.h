#ifndef DATE_H
#define DATE_H

#include "cm_types.h"

/*
 * The date of Date.cpp (si\code), 8 bytes as in the records of the
 * database. `day` is the day of the year, 0 for 1 Jan. The original writes
 * `leap` as an int, and the weekday reads it as an int, so a value in the
 * upper half also counts as a leap year.
 */
typedef struct {
    s16 day;
    u16 year;
    u32 leap;
} cm_date_t;

_Static_assert(sizeof(cm_date_t) == 8, "a date is 8 bytes");

/* The days of the week that the weekday functions use. */
enum { DATE_MONDAY = 0, DATE_SATURDAY = 5, DATE_SUNDAY = 6 };

/*
 * FUN_00549f70, the constructor: day 1 to 31 of month 0 (January) to 11 of
 * `year`. A day or month out of range shows an error box (Date.cpp 0x34,
 * 0x3b), clears g_left_held and becomes 1 or 0. The day is not checked
 * against the month, so 31 Feb is 3 Mar. With `weekday` not -1, the date
 * moves to the nearest day with that weekday (date_to_weekday). Returns `d`.
 */
cm_date_t *date_init(cm_date_t *d, s16 day, s8 month, u16 year, s16 weekday);

/* FUN_0054a130. As date_init, with error lines 0x72 and 0x79. */
void date_set(cm_date_t *d, s16 day, s8 month, u16 year, s16 weekday);

/* FUN_0054a110. The default date of the records: 10 Feb 1972. */
void date_default(cm_date_t *d);

/* FUN_00417c10. Copies `src` into `d` and returns the copy. */
cm_date_t date_copy(cm_date_t *d, const cm_date_t *src);

/* FUN_0054a3f0. 0 for Monday to 6 for Sunday. Before 1600 also -6 to -1. */
s16 date_weekday(const cm_date_t *d);

/*
 * FUN_0054a2d0. Writes to `out` the day with `weekday` that is at most three
 * days from `d`. `out` may be `d`.
 */
void date_to_weekday(const cm_date_t *d, s16 weekday, cm_date_t *out);

/*
 * FUN_0054c5b0 and FUN_0054c8e0. The date `days` later or earlier. A
 * negative count goes the other way. The day sum wraps at 16 bits.
 */
cm_date_t date_add_days(const cm_date_t *d, s16 days);
cm_date_t date_sub_days(const cm_date_t *d, s16 days);

/* FUN_0054c770 and FUN_0054cab0. The date `weeks` weeks later or earlier. */
cm_date_t date_add_weeks(const cm_date_t *d, u8 weeks);
cm_date_t date_sub_weeks(const cm_date_t *d, u8 weeks);

#endif
