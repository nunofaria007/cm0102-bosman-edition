#include "langlib.h"
#include <stdarg.h>
#include <stdio.h>

#define FORMAT_MAX 0x828 /* the buffer of the stripped format, 0xb6619c */
#define RESULT_MAX 2048

char g_text_buffer[0x834];
char g_title_buffer[0x70];

const char *langlib_version(void)
{
    return "3.9.68"; /* 0xad4394 */
}

/*
 * The loop at 0x66f7fe that builds the format for vsprintf. Where the
 * original reads past the end of a format with a "<" note that has no ">",
 * the port stops at the end.
 */
static void strip_notes(char *out, const char *s)
{
    s32 k = 0, j = 0;
    bool in_note = false;

    if (s[0] == '\0') {
        out[0] = '\0';
        return;
    }
    for (;;) {
        char c = s[k];

        if (c == '<') {
            while (c != '\0') {
                c = s[++k];
                if (c == '%') break;
            }
            in_note = true;
        }
        c = s[k];
        if (c == ' ' && in_note) {
            while (c != '\0') {
                c = s[++k];
                if (c == '>') break;
            }
            if (c != '\0') c = s[++k];
            in_note = false;
        }
        if (j < FORMAT_MAX - 1) out[j++] = c;
        if (s[k] == '\0' || s[++k] == '\0') break;
    }
    out[j < FORMAT_MAX ? j : FORMAT_MAX - 1] = '\0';
}

char *text_format(char *dst, const char *format, ...)
{
    char stripped[FORMAT_MAX];
    s32 n = 0;
    va_list args;

    while (format[n] == ' ') {
        dst[n] = ' ';
        n++;
    }
    strip_notes(stripped, format + n);
    va_start(args, format);
    vsnprintf(dst + n, RESULT_MAX, stripped, args);
    va_end(args);
    return dst + n;
}
