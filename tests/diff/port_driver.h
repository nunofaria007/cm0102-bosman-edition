#ifndef PORT_DRIVER_H
#define PORT_DRIVER_H

#include "cm_types.h"

/* Prints "0" for no image, else the header fields and then the raw pixels. */
void print_image(const cm_image_t *img);

#endif
