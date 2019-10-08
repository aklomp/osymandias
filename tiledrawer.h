#pragma once

#include <stdint.h>

#include "cache.h"
#include "camera.h"
#include "texture_cache.h"
#include "viewport.h"

struct tiledrawer {
	const struct cache_node    *tile;
	const struct texture_cache *tex;
};

extern void tiledrawer       (const struct tiledrawer *);
extern void tiledrawer_start (const struct camera *, const struct viewport *);
