/* A small PDF writer for printouts (#33). It writes fixed-width text pages in Courier, with no
 * other library. The text is in code page 1252 and the PDF uses WinAnsiEncoding, so the bytes pass
 * as they are. */
#ifndef RECOMP_PDF_H
#define RECOMP_PDF_H

#include <stddef.h>

/* Builds a PDF from text (len bytes). Lines end with '\n'. A form feed '\f' at the start of a
 * line, or alone on its line, starts a new page. A page also breaks after lines_per_page lines
 * (0 means no limit other than the sheet). The font shrinks, down to 6 points, so that the longest
 * line fits the width of an A4 sheet. Returns a block from malloc, with its size in *out_len, or
 * NULL when memory fails. The caller frees it. */
char *pdf_build(const char *text, size_t len, int lines_per_page, size_t *out_len);

/* The file name of a printout: "<kind> - <club> <yyyy>-<mm>-<dd>.pdf". The club and the kind are
 * UTF-8. Characters that a file name cannot hold become a space, and the club is cut at 40 bytes
 * (at a character end). Returns 1 when it fits in room. */
int pdf_file_name(char *out, size_t room, const char *kind, const char *club, int year, int month, int day);

#endif
