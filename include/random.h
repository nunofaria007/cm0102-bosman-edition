#ifndef RANDOM_H
#define RANDOM_H

#include "cm_types.h"
#include <stdbool.h>
#include <stddef.h>

/*
 * The random numbers of utils.cpp (si\code). The main generator draws from a
 * table of 51,000 numbers in the data of cm0102.exe (0xaa2454). The repo
 * holds none of it: random_load_table reads it from the player's own exe.
 */
#define RANDOM_TABLE_SIZE 51000

/* The state. A differential test sets and compares all four. */
extern u32 g_holdrand;          /* DAT_00ad6c30, the state of rand */
extern s32 g_random_index;      /* DAT_00de1f60, as an index into the table */
extern s32 g_random_end;        /* DAT_00de2798, as an index: -1 before the seed */
extern s32 g_random_offset;     /* DAT_00de1f5c, added to each table number */
extern s32 g_random_table[RANDOM_TABLE_SIZE];

/* DAT_00de1f64, "Random Number seed: %d" after random_seed. */
extern char g_random_seed_text[64];

/* 0x9469fa, srand of MSVC. */
void cm_srand(u32 seed);

/* 0x946a04, rand of MSVC: 0 to 0x7fff. */
s32 cm_rand(void);

/*
 * FUN_0090cfc0. A number from 0 to n - 1, or from n + 1 to 0 for a negative
 * n. 0 for n 0. Each call takes the next table number. A count past 0xffff
 * takes two or more, and uses a double.
 */
s32 random_number(s32 n);

/*
 * FUN_0090d0a0. Seeds rand with `seed`, or with the time when it is 0, and
 * picks a start in the table and an offset with rand.
 */
void random_seed(s32 seed);

/* Copies the table from the loaded image (exe_image_load). False if none. */
bool random_use_image(void);

/*
 * Loads the image of cm0102.exe at `exe_path` (exe_image.h), then copies the
 * table from it. The file must have the SHA256 of the 3.9.68 exe. On
 * failure, writes a message for the player to `error` and returns false.
 */
bool random_load_table(const char *exe_path, char *error, size_t error_size);

#endif
