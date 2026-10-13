#ifndef DEMO_H
#define DEMO_H

#include "cm_types.h"

/*
 * A made-up screen that shows the ported area, widget and text code at
 * work. It is a developer aid. Nothing here comes from the original.
 */

/* Draws the screen. `background` is an RGN file, or NULL for the default picture. */
void demo_draw(const char *data_dir, const char *background);

/* Scrolls the table by a number of rows and draws it again. */
void demo_scroll(s32 rows);

#endif
