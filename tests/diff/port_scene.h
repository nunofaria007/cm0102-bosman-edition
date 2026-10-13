#ifndef PORT_SCENE_H
#define PORT_SCENE_H

#include "cm_types.h"

cm_image_t *rgn_load(const char *path);
void rgn_free(cm_image_t *img);

/* Prints "0" for no image, else the header fields and then the raw pixels. */
void print_image(const cm_image_t *img);

/* Runs a command that works on the areas and widgets of the test scene. */
bool scene_command(const char *cmd, const char *args);

#endif
