#ifndef UTILS_H
#define UTILS_H

#include "cm_types.h"

/* The Pictures folder of the original. The app sets it. */
extern const char *g_pictures_dir;

/*
 * FUN_0090bde0. The path of a file in a folder of the original: "CM3_DATA"
 * (g_data_dir) or "CM3_PICTURES" (g_pictures_dir). A backslash in `name`
 * separates folders, as on Windows. Each part matches a file or folder of
 * any case. Returns a static buffer that the next call overwrites.
 */
const char *utils_path(const char *folder, const char *name);

#endif
