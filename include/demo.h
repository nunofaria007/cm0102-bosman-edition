#ifndef DEMO_H
#define DEMO_H

#include "cm_types.h"

/*
 * A made-up screen that shows the ported area, widget and text code at
 * work. It is a developer aid. Nothing here comes from the original.
 */

/*
 * Builds the screen in a screen manager and draws it. `background` is an RGN
 * file. NULL gives DEFAULT_PIC.RGN of the data folder (g_data_dir).
 */
void demo_draw(const char *background);

/* Scrolls the table by a number of rows and draws it again. */
void demo_scroll(s32 rows);

#endif
