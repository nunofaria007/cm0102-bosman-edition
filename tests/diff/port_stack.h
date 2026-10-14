#ifndef PORT_STACK_H
#define PORT_STACK_H

#include <stdbool.h>

/* Runs a command that works on the screen stack of the port driver. */
bool stack_command(const char *cmd, const char *args);

#endif
