#pragma once

bool diskcache_add (unsigned int zoom, int tile_x, int tile_y, const char *data, size_t size);
bool diskcache_del (unsigned int zoom, int tile_x, int tile_y);
int diskcache_open (unsigned int zoom, int tile_x, int tile_y);
