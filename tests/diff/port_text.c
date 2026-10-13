/*
 * Stands in for the text function of the port in the port driver. It records
 * each call, so the differential test can compare the calls with those the
 * original makes to FUN_005e6d80.
 */
#include "port_text.h"
#include "gui_text.h"
#include <stdio.h>
#include <string.h>

#define MAX_CALLS 64

static char calls[MAX_CALLS][1024];
static int call_count;

void gui_text(s32 x1, s32 y1, s32 x2, s32 y2, u32 flags, u16 colour,
              u16 second_colour, const char *text, s32 param)
{
    char *out;

    if (call_count == MAX_CALLS) return;
    out = calls[call_count++];
    out += sprintf(out, "%d %d %d %d %u %u %u %d -", x1, y1, x2, y2, flags,
                   colour, second_colour, param);
    for (size_t i = 0; i < strlen(text) && i < 400; i++) {
        out += sprintf(out, "%02x", (unsigned char)text[i]);
    }
}

/* "text_calls": the number of calls since the last time, then one line each. */
void text_print_calls(void)
{
    printf("%d\n", call_count);
    for (int i = 0; i < call_count; i++) printf("%s\n", calls[i]);
    call_count = 0;
}
