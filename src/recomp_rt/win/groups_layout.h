/* Attribute groups (#43): the layout of the attribute table of the player profile, without game memory,
 * so that tests/groups/test_groups.c can test it (spec 2026-10-15-features-0.4.0, Q122 to Q129). */
#ifndef RECOMP_GROUPS_LAYOUT_H
#define RECOMP_GROUPS_LAYOUT_H

#define GROUP_COLUMNS 3
#define GROUP_ROWS 13            /* the rows of a column with heading rows */
#define GROUP_ROWS_MIN 12        /* the rows of the original: 36 cells in 3 columns */

/* The kinds of cell. 1 to 31 are the attribute numbers of the game's getter (names: 0x5428a0), in
 * alphabetical order of their English names. */
enum {
    CELL_BLANK = 0,
    ATTR_ACCELERATION = 1, ATTR_AGGRESSION, ATTR_AGILITY, ATTR_ANTICIPATION, ATTR_BALANCE, ATTR_BRAVERY,
    ATTR_CREATIVITY, ATTR_CROSSING, ATTR_DECISIONS, ATTR_DETERMINATION, ATTR_DRIBBLING, ATTR_FINISHING,
    ATTR_FLAIR, ATTR_HANDLING, ATTR_HEADING, ATTR_INFLUENCE, ATTR_JUMPING, ATTR_LONG_SHOTS, ATTR_MARKING,
    ATTR_OFF_THE_BALL, ATTR_PACE, ATTR_PASSING, ATTR_POSITIONING, ATTR_REFLEXES, ATTR_SET_PIECES,
    ATTR_STAMINA, ATTR_STRENGTH, ATTR_TACKLING, ATTR_TEAMWORK, ATTR_TECHNIQUE, ATTR_WORK_RATE,
    /* The five rows that the original adds after the sorted attributes, in its order. */
    CELL_FOOT = 32, CELL_FORM, CELL_MORALE, CELL_CONDITION, CELL_ON_LOAN,
    /* Heading rows. */
    CELL_HEAD_GOALKEEPING, CELL_HEAD_TECHNICAL, CELL_HEAD_MENTAL, CELL_HEAD_PHYSICAL
};

typedef struct {
    int rows;                                  /* rows in each column */
    int cell[GROUP_COLUMNS][GROUP_ROWS];
    const char *title[GROUP_COLUMNS];          /* the group names of a column without its heading rows,
                                                * or NULL */
} GroupLayout;

/* Fills out for a goalkeeper (keeper 1) or an outfield player, when fit rows fit the area. A column
 * whose cells and heading rows do not fit loses its heading rows, and its group names go to its title
 * (Q125). Returns the rows of each column: GROUP_ROWS or GROUP_ROWS_MIN. */
int groups_layout(int keeper, int fit, GroupLayout *out);

/* The text of a heading cell, or NULL for any other cell. */
const char *groups_heading(int cell);

#endif
