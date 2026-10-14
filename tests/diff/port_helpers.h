#ifndef PORT_HELPERS_H
#define PORT_HELPERS_H

#include <stdbool.h>

#define TEXT_SIZE 2048

/* Runs a command for the helpers of the screens, on g_screen_stack. */
bool helpers_command(const char *cmd, const char *args);

/* Reads the next number and moves the arguments on. */
long next_long(const char **args);

/* Reads "-hex" into one of a few static buffers, or "null" as NULL. */
char *next_text(const char **args);

/* Prints "-" and two hex digits for each byte. */
void print_hex(const char *text);

#endif
