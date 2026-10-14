#ifndef PORT_ENTRY_H
#define PORT_ENTRY_H

#include <stdbool.h>

/* Runs a command for text entry on the screen manager of the test scene. */
bool entry_command(const char *cmd, const char *args);

#endif
