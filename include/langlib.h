#ifndef LANGLIB_H
#define LANGLIB_H

#include "cm_types.h"
#include <stddef.h>

/*
 * Text of the original, from Langlib.cpp. The original looks a format up in
 * its language table when its language index (DAT_009eb2dc) is not -1 or 0.
 * The port has no language table and always takes the plain path.
 */

/* Scratch buffers that many screens of the original format their text into. */
extern char g_text_buffer[0x834];   /* DAT_00de1f64 */
extern char g_title_buffer[0x70];   /* DAT_00dd70a8 */

/* FUN_0090d130. The version that the side bar shows. */
const char *langlib_version(void);

/*
 * FUN_0066f4e0, the plain path. Copies the leading spaces of `format` to
 * `dst`, then formats the rest after them with the printf rules of MSVC. A
 * "<" starts a note for the translator: the text from "<" up to the next "%"
 * goes, and later the text from the next space up to and including the next
 * ">". "Version\n<%s - version>" becomes "Version\n%s". Returns `dst` plus
 * the count of leading spaces.
 *
 * The tags "{...}" and "[...]" are not ported: the port keeps their
 * characters as plain text. The original drops an empty tag, skips one
 * argument for each "{" or "[", and replaces a tag with content in a later
 * pass. The original has no limit on the result. The port writes at most
 * `size` - 1 characters, the leading spaces included.
 */
char *text_format(char *dst, size_t size, const char *format, ...)
    __attribute__((format(printf, 3, 4)));

#endif
