#pragma once

#include <stdbool.h>

#include "cache.h"

// Request data from the bitmap cache. Before calling this function, the user
// must lock the cache. The cache must be unlocked only after the returned data
// has been used, or there is a risk of race conditions with the threadpool.
extern void *bitmap_cache_search (const struct cache_node *in, struct cache_node *out);

extern void bitmap_cache_lock   (void);
extern void bitmap_cache_unlock (void);

extern void bitmap_cache_destroy (void);
extern bool bitmap_cache_create  (void);
