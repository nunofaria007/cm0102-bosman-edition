#ifndef PORT_SCENE_H
#define PORT_SCENE_H

#include "scrman.h"
#include <stdbool.h>

/* Runs a command that works on the areas and widgets of the test scene. */
bool scene_command(const char *cmd, const char *args);

/* The screen manager of the test scene. */
scrman_t *scene_manager(void);

#endif
