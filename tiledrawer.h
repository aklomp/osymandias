#pragma once

#include <GL/gl.h>

#include "cache.h"

struct tiledrawer {
	const struct cache_node *tile;
	unsigned int world_zoom;
	GLuint texture_id;
};

extern void tiledrawer       (const struct tiledrawer *);
extern void tiledrawer_start (void);
