#ifndef PORT_DATABASE_H
#define PORT_DATABASE_H

#include <stdbool.h>

/* Runs a command for the database load. */
bool database_command(const char *cmd, const char *args);

#endif
