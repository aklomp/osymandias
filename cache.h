#pragma once

#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>

struct cache;

struct cache_config {

	// Total maximum number of active cache entries.
	size_t capacity;

	// Size of one cache entry in bytes.
	size_t entrysize;

	// Callback function to call when an entry is purged.
	void (* destroy) (void *);
};

struct cache_node {
	union {
		struct {
			uint32_t x;
			uint32_t y;
		} __attribute__((packed));
		uint64_t key;
	};
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
extern void *cache_insert (struct cache *cache, const struct cache_node *loc, void *data);

// Retrieve data from the cache at a given level. The function returns data at
// this zoom level or lower. The #out member describes the returned node.
extern void *cache_search (struct cache *cache, const struct cache_node *in, struct cache_node *out);

// Cache creation/destruction.
extern void cache_destroy (struct cache *cache);
extern struct cache *cache_create (const struct cache_config *config);
