#pragma once

#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>

struct cache;

// Union of data types that can be stored in the cache.
union cache_data {
	void    *ptr;
	uint32_t u32;
};

struct cache_config {

	// Total maximum number of active cache members.
	size_t capacity;

	// Callback function to call when an element is purged.
	void (* destroy) (union cache_data *);
};

struct cache_node {
	uint32_t x;
	uint32_t y;
	uint32_t zoom;
};

// Move up one zoom level, halving coordinates.
static inline bool cache_node_up (struct cache_node *n)
{
	if (n->zoom == 0)
		return false;

	n->zoom--;
	n->x >>= 1;
	n->y >>= 1;
	return true;
}

// Insert opaque data into the cache at a given level. If a node exists for the
// location, it is reused. If the insertion would exceed the cache capacity,
// the least active cache node is purged first to make space for the insertion.
extern void cache_insert (struct cache *cache, const struct cache_node *loc, union cache_data *data);

// Retrieve data from the cache at a given level. The function returns data at
// this zoom level or lower. The #out member describes the returned node.
extern union cache_data *cache_search (struct cache *cache, const struct cache_node *in, struct cache_node *out);

// Cache creation/destruction.
extern void cache_destroy (struct cache *cache);
extern struct cache *cache_create (const struct cache_config *config);
