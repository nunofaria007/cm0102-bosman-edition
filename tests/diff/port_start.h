#ifndef PORT_START_H
#define PORT_START_H

#include <stdbool.h>

/* Runs a command for the start screen, its helpers and the screen stack g_screen_stack. */
bool start_command(const char *cmd, const char *args);

#endif
