#pragma once

#include <stdbool.h>
#include <stddef.h>

extern bool png_load (const char *data, size_t len, unsigned int *height, unsigned int *width, char **rawbits);
