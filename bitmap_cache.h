#pragma once

#include <stdbool.h>

#include "cache.h"
#include "globe.h"

// Data structure stored in and retrieved from the bitmap cache.
struct bitmap_cache {
	struct globe_tile coords;
	void             *rgb;
};

// Insert an entry into to the bitmap cache.
extern void bitmap_cache_insert (const struct cache_node *loc, void *rgb);

// Request data from the bitmap cache. Before calling this function, the user
// must lock the cache. The cache must be unlocked only after the returned data
// has been used, or there is a risk of race conditions with the threadpool.
extern const struct bitmap_cache *bitmap_cache_search (const struct cache_node *in, struct cache_node *out);

extern void bitmap_cache_lock   (void);
extern void bitmap_cache_unlock (void);

extern void bitmap_cache_destroy (void);
extern bool bitmap_cache_create  (void);
