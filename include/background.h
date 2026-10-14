#ifndef BACKGROUND_H
#define BACKGROUND_H

#include "cm_types.h"

/*
 * The background pictures of background.cpp: the table that Pictures/pics.cfg
 * fills, and the random pick for a screen.
 */

#define PICS_SECTIONS 11

/* One "key: name, name, ..." line of pics.cfg, 16 bytes in the original. */
typedef struct {
    char *key;      /* 0x00 */
    s16 count;      /* 0x04, of names */
    s16 count2;     /* 0x06, of names2 */
    char **names;   /* 0x08 */
    char **names2;  /* 0x0c, the names that a "{" ends */
} pics_entry_t;

/*
 * One "[name]" section, 10 bytes in the original (0x989860). A section can
 * keep its entry array after its last entry goes, with count 0.
 */
typedef struct {
    const char *name;     /* 0x00 */
    s16 count;            /* 0x04 */
    pics_entry_t *entries; /* 0x06 */
} pics_section_t;

/*
 * Sections 0 to 10: nations, clubs, stadiums, comps, awards, staff,
 * training, boardroom, trophy, transfer, si.
 */
extern pics_section_t g_pics[PICS_SECTIONS];

/*
 * FUN_0041b3d0. Reads pics.cfg of g_pictures_dir into g_pics, after what an
 * earlier call read. A name stays only when its file opens: "name", then
 * "stadium\name", then "old\name", then the same three with "_name" when the
 * name does not start with "_". The kept name is the form that opened.
 * Line ends stay part of the last name of a line, as in the original.
 * The original ends the program when it runs out of memory. The port stops
 * reading.
 */
void pics_load(void);

/* Frees g_pics and empties it. Not in the original, which loads it once. */
void pics_free(void);

/*
 * FUN_0041bfb0. When the title of the current screen is "\" or empty, makes
 * it the path of a random picture of sections 0 to 5, except stadiums. With
 * no picture, the title becomes ":". Any other title stays, so a rebuild
 * keeps its picture. The original takes its random numbers from a pool in
 * its data. The port uses rand(). The first call loads pics.cfg when no
 * pics_load came before.
 */
void pick_background(void);

#endif
