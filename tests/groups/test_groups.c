/* Test of the attribute groups layout (src/recomp_rt/win/groups_layout.c).
 * Usage: groups_test */
#include "groups_layout.h"

#include <stdio.h>
#include <string.h>

static int failures;

#define CHECK(cond) do { if (!(cond)) { printf("FAIL line %d: %s\n", __LINE__, #cond); failures++; } } while (0)

/* The English names of the game's attributes 1 to 31 (0x5428a0), for the alphabetical check. */
static const char *names[32] = {"", "Acceleration", "Aggression", "Agility", "Anticipation", "Balance",
    "Bravery", "Creativity", "Crossing", "Decisions", "Determination", "Dribbling", "Finishing", "Flair",
    "Handling", "Heading", "Influence", "Jumping", "Long Shots", "Marking", "Off The Ball", "Pace", "Passing",
    "Positioning", "Reflexes", "Set Pieces", "Stamina", "Strength", "Tackling", "Teamwork", "Technique",
    "Work Rate"};

/* 1 when column c holds exactly the cells of want (ended by -1), then blanks. */
static int column_is(const GroupLayout *l, int c, const int *want)
{
    int r = 0;
    for (; want[r] >= 0; r++)
        if (r >= l->rows || l->cell[c][r] != want[r]) return 0;
    for (; r < l->rows; r++)
        if (l->cell[c][r] != CELL_BLANK) return 0;
    return 1;
}

/* How often cell occurs in the layout. */
static int occurs(const GroupLayout *l, int cell)
{
    int n = 0;
    for (int c = 0; c < GROUP_COLUMNS; c++)
        for (int r = 0; r < l->rows; r++) n += l->cell[c][r] == cell;
    return n;
}

/* 1 when the attributes between headings of each column are in alphabetical order. */
static int alphabetical(const GroupLayout *l)
{
    for (int c = 0; c < GROUP_COLUMNS; c++) {
        const char *last = "";
        for (int r = 0; r < l->rows; r++) {
            int k = l->cell[c][r];
            if (groups_heading(k)) last = "";
            else if (k >= 1 && k <= 31) {
                if (strcmp(last, names[k]) >= 0) return 0;
                last = names[k];
            }
        }
    }
    return 1;
}

int main(void)
{
    GroupLayout l;

    /* 1. Outfield player, 13 rows fit: Technical, Mental, Physical with heading rows (Q122, Q124). */
    CHECK(groups_layout(0, 13, &l) == GROUP_ROWS && l.rows == GROUP_ROWS);
    static const int tech[] = {CELL_HEAD_TECHNICAL, ATTR_CROSSING, ATTR_DRIBBLING, ATTR_FINISHING, ATTR_HEADING,
        ATTR_LONG_SHOTS, ATTR_MARKING, ATTR_PASSING, ATTR_SET_PIECES, ATTR_TACKLING, ATTR_TECHNIQUE, -1};
    static const int mental[] = {CELL_HEAD_MENTAL, ATTR_AGGRESSION, ATTR_ANTICIPATION, ATTR_BRAVERY,
        ATTR_CREATIVITY, ATTR_DECISIONS, ATTR_DETERMINATION, ATTR_FLAIR, ATTR_INFLUENCE, ATTR_OFF_THE_BALL,
        ATTR_POSITIONING, ATTR_TEAMWORK, ATTR_WORK_RATE, -1};
    static const int physical[] = {CELL_HEAD_PHYSICAL, ATTR_ACCELERATION, ATTR_AGILITY, ATTR_BALANCE,
        ATTR_JUMPING, ATTR_PACE, ATTR_STAMINA, ATTR_STRENGTH, CELL_FOOT, CELL_FORM, CELL_MORALE,
        CELL_CONDITION, CELL_ON_LOAN, -1};
    CHECK(column_is(&l, 0, tech));
    CHECK(column_is(&l, 1, mental));
    CHECK(column_is(&l, 2, physical));
    CHECK(!l.title[0] && !l.title[1] && !l.title[2]);
    CHECK(occurs(&l, ATTR_HANDLING) == 0 && occurs(&l, ATTR_REFLEXES) == 0);
    for (int k = 1; k <= 31; k++)
        if (k != ATTR_HANDLING && k != ATTR_REFLEXES) CHECK(occurs(&l, k) == 1);
    CHECK(alphabetical(&l));

    /* 2. Goalkeeper, 13 rows fit: Goalkeeping, then a short Technical group (Q129). */
    CHECK(groups_layout(1, 13, &l) == GROUP_ROWS);
    static const int keeper[] = {CELL_HEAD_GOALKEEPING, ATTR_HANDLING, ATTR_REFLEXES, ATTR_PASSING,
        CELL_HEAD_TECHNICAL, ATTR_SET_PIECES, ATTR_TECHNIQUE, -1};
    CHECK(column_is(&l, 0, keeper));
    CHECK(column_is(&l, 1, mental));
    CHECK(column_is(&l, 2, physical));
    static const int hidden[] = {ATTR_CROSSING, ATTR_DRIBBLING, ATTR_FINISHING, ATTR_HEADING, ATTR_LONG_SHOTS,
        ATTR_MARKING, ATTR_TACKLING};
    for (int i = 0; i < 7; i++) CHECK(occurs(&l, hidden[i]) == 0);
    CHECK(!l.title[0] && !l.title[1] && !l.title[2]);

    /* 3. Classic: 12 rows fit. A column that does not fit with its heading rows keeps 12 cells and gets
     *    its group name as its title (Q125). Technical fits with its heading row. */
    CHECK(groups_layout(0, 12, &l) == GROUP_ROWS_MIN);
    CHECK(column_is(&l, 0, tech));
    CHECK(column_is(&l, 1, mental + 1) && l.title[1] && !strcmp(l.title[1], "Mental"));
    CHECK(column_is(&l, 2, physical + 1) && l.title[2] && !strcmp(l.title[2], "Physical"));
    CHECK(!l.title[0]);
    CHECK(groups_layout(1, 12, &l) == GROUP_ROWS_MIN);
    CHECK(column_is(&l, 0, keeper) && !l.title[0]);

    /* 4. More room than 13 rows still gives 13; less than 12 gives the original 12. */
    CHECK(groups_layout(0, 19, &l) == GROUP_ROWS);
    CHECK(groups_layout(0, 9, &l) == GROUP_ROWS_MIN);

    /* 5. The heading texts. */
    CHECK(!strcmp(groups_heading(CELL_HEAD_GOALKEEPING), "Goalkeeping"));
    CHECK(!strcmp(groups_heading(CELL_HEAD_TECHNICAL), "Technical"));
    CHECK(groups_heading(ATTR_PACE) == NULL && groups_heading(CELL_BLANK) == NULL);

    if (failures) printf("%d failures\n", failures);
    else printf("groups: all checks passed\n");
    return failures != 0;
}
