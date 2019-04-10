#pragma once

#include <stdint.h>

#include "cache.h"

struct tiledrawer {
	const struct cache_node *tile;
	uint32_t world_zoom;
	uint32_t texture_id;
};

extern void tiledrawer       (const struct tiledrawer *);
extern void tiledrawer_start (void);
