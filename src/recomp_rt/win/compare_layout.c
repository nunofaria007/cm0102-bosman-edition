/* The comparison's axes and rows: see compare_layout.h. */
#include "compare_layout.h"

#include <math.h>
#include <stdio.h>

static const CompareAxis outfield[COMPARE_AXES] = {
    {"Defending", "Def", {ATTR_TACKLING, ATTR_MARKING, ATTR_POSITIONING}},
    {"Physical", "Phy", {ATTR_STRENGTH, ATTR_STAMINA, ATTR_AGILITY, ATTR_BALANCE}},
    {"Speed", "Spd", {ATTR_PACE, ATTR_ACCELERATION}},
    {"Vision", "Vis", {ATTR_PASSING, ATTR_CREATIVITY, ATTR_FLAIR}},
    {"Attacking", "Att", {ATTR_FINISHING, ATTR_OFF_THE_BALL, ATTR_LONG_SHOTS}},
    {"Technique", "Tec", {ATTR_TECHNIQUE, ATTR_DRIBBLING, ATTR_CROSSING}},
    {"Aerial", "Aer", {ATTR_HEADING, ATTR_JUMPING}},
    {"Mental", "Men", {ATTR_DECISIONS, ATTR_ANTICIPATION, ATTR_WORK_RATE, ATTR_TEAMWORK, ATTR_DETERMINATION}},
};

/* The goalkeeper axes (proposal of slice 6, for Max's approval): only attributes that a goalkeeper's
 * profile shows (Q129). Shot Stopping, Handling, Aerial and Command hold the picker's goalkeeper keys
 * (Handling, Reflexes, Agility, Positioning, Jumping); Speed, Physical and Mental are as for outfield
 * players without Agility, which Shot Stopping holds. */
static const CompareAxis keeper[COMPARE_AXES] = {
    {"Shot Stopping", "Sho", {ATTR_REFLEXES, ATTR_AGILITY}},
    {"Handling", "Han", {ATTR_HANDLING}},
    {"Aerial", "Aer", {ATTR_JUMPING, ATTR_BRAVERY}},
    {"Command", "Com", {ATTR_POSITIONING, ATTR_INFLUENCE}},
    {"Distribution", "Dis", {ATTR_PASSING, ATTR_TECHNIQUE}},
    {"Speed", "Spd", {ATTR_PACE, ATTR_ACCELERATION}},
    {"Physical", "Phy", {ATTR_STRENGTH, ATTR_STAMINA, ATTR_BALANCE}},
    {"Mental", "Men", {ATTR_DECISIONS, ATTR_ANTICIPATION, ATTR_WORK_RATE, ATTR_TEAMWORK, ATTR_DETERMINATION}},
};

const CompareAxis *compare_axes(int keepers) { return keepers ? keeper : outfield; }

int compare_axis_value(const CompareAxis *a, const uint8_t *attr)
{
    int sum = 0, n = 0;
    for (int i = 0; a->attr[i]; i++)
        if (attr[a->attr[i]]) {
            sum += attr[a->attr[i]];
            n++;
        }
    return n ? (sum * 20 + n) / (2 * n) : -1;
}

int compare_rows(int keepers, int *rows, int max)
{
    GroupLayout l;
    groups_layout(keepers, &l);
    int n = 0;
    for (int c = 0; c < GROUP_COLUMNS; c++)
        for (int r = 0; r < GROUP_ROWS; r++) {
            int k = l.cell[c][r];
            if ((k >= 1 && k <= 31) || groups_heading(k))
                if (n < max) rows[n++] = k;
        }
    return n;
}

void compare_corner(int i, int tenths, int cx, int cy, int r, int *x, int *y)
{
    double a = -M_PI / 2 + i * 2 * M_PI / COMPARE_AXES, d = tenths < 0 ? 0 : r * tenths / 200.0;
    *x = cx + (int)lround(cos(a) * d);
    *y = cy + (int)lround(sin(a) * d);
}

void compare_value_text(int tenths, char *out, size_t size)
{
    if (tenths < 0) snprintf(out, size, "-");
    else snprintf(out, size, "%d.%d", tenths / 10, tenths % 10);
}
