#pragma once

#include <stdbool.h>
#include <stddef.h>

extern bool diskcache_add  (unsigned int zoom, int tile_x, int tile_y, const char *data, size_t size);
extern bool diskcache_del  (unsigned int zoom, int tile_x, int tile_y);
extern int  diskcache_open (unsigned int zoom, int tile_x, int tile_y);
