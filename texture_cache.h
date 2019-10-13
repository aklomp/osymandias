#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "bitmap_cache.h"
#include "cache.h"
#include "globe.h"

// Data structure stored in and retrieved from the texture cache.
struct texture_cache {
	struct globe_tile coords;
	uint32_t          id;
};

extern const struct texture_cache *texture_cache_search (const struct cache_node *in, struct cache_node *out);
extern const struct texture_cache *texture_cache_insert (const struct cache_node *loc, const struct bitmap_cache *bitmap);

extern void texture_cache_destroy (void);
extern bool texture_cache_create  (void);
