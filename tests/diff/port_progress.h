#ifndef PORT_PROGRESS_H
#define PORT_PROGRESS_H

#include <stdbool.h>

/* Runs a command for the progress box and opcode 7, on g_screen_stack. */
bool progress_command(const char *cmd, const char *args);

#endif
