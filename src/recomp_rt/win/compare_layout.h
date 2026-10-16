/* The comparison of two players (#22, spec 2026-10-15-features-0.4.0, slice 6): the polygon's axes, the
 * rows of each player's attribute list and the polygon's geometry, without game memory, so that
 * tests/compare/test_compare.c can test them. */
#ifndef RECOMP_COMPARE_LAYOUT_H
#define RECOMP_COMPARE_LAYOUT_H

#include <stddef.h>
#include <stdint.h>

#include "groups_layout.h"

#define COMPARE_AXES 8
#define COMPARE_ROWS 40          /* more than the rows of either list */

/* An axis: its name, a short name of three letters for a small polygon (classic mode) and its attributes
 * (1 to 31, the getter's numbers), ended by 0. */
typedef struct {
    const char *name, *abbr;
    int attr[6];
} CompareAxis;

/* The eight axes, clockwise from the top: the outfield axes (Q110, Q117), or the goalkeeper axes when
 * both players are goalkeepers (keepers 1). */
const CompareAxis *compare_axes(int keepers);

/* The value of an axis for a player in tenths (0 to 200): the average of its attributes, rounded half
 * up. attr[k] is the shown value 1 to 20 of attribute k, 0 when the viewer's screens hide it; a hidden
 * attribute does not count (Q111). -1 when every attribute of the axis is hidden. */
int compare_axis_value(const CompareAxis *a, const uint8_t *attr);

/* The rows of each player's list: the profile's attribute groups (groups_layout) with their heading
 * cells, column after column, without the profile's other rows and blanks. Goalkeeper groups when both
 * players are goalkeepers. Returns the count. */
int compare_rows(int keepers, int *rows, int max);

/* The corner of axis i for a value in tenths on a polygon around (cx, cy) whose value 20 is radius r.
 * Axis 0 points up, the others follow clockwise. A value of -1 gives the centre. */
void compare_corner(int i, int tenths, int cx, int cy, int r, int *x, int *y);

/* "12.3" for 123, "-" for -1. */
void compare_value_text(int tenths, char *out, size_t size);

#endif
