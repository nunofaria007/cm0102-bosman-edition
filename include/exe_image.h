#ifndef EXE_IMAGE_H
#define EXE_IMAGE_H

#include "cm_types.h"
#include <stdbool.h>
#include <stddef.h>

/*
 * The image of the player's own cm0102.exe 3.9.68. The repo holds none of its
 * data: the port reads what it needs (the random table, the names of the key
 * ids) from this image at start, by the address that the original has.
 */

/*
 * Reads the file at `exe_path` and checks its SHA256. Replaces the image on
 * success only. On failure, writes a message for the player to `error` and
 * returns false.
 */
bool exe_image_load(const char *exe_path, char *error, size_t error_size);

/* True after a successful exe_image_load. */
bool exe_image_loaded(void);

/* The bytes of the file. Valid after exe_image_load. */
const u8 *exe_image_file(size_t *size);

/*
 * The C string at an address of the loaded image (0x400000 and up), or NULL
 * if the address is outside the file data of the sections. Stops the program
 * with a message if no image is loaded.
 */
const char *exe_image_string(u32 address);

#endif
