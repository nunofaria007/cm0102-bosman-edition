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

/*
 * FUN_0090c1d0. Compares two names byte by byte as unsigned values, after it
 * maps each byte through the table at 0xaa2154, which removes accents ("e"
 * for 0xe9). With `keep_case` 0 it then maps each byte to lower case.
 * g_language_index 10 skips the first map. `max_len` > 0 stops after that
 * many equal bytes. A NULL name comes first. Returns -1, 0 or 1.
 */
s32 utils_name_compare(const char *a, const char *b, s32 keep_case, u32 max_len);

#endif
