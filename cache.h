#pragma once

#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>

struct cache;

// Structure to hold coordinates in 3D space for the tile.
struct cache_data_coords {
	float x;
	float y;
	float z;
} __attribute__((packed));

// Union of data types that can be stored in the cache.
struct cache_data {
	union {
		void    *ptr;
		uint32_t u32;
	};

	// 3D spherical coordinates of the tile. These are cached because they
	// are calculated by the host in double precision (which is relatively
	// expensive), and because they remain constant for the lifetime of the
	// tile. It would be possible to calculate these coordinates in a
	// shader, but some video cards do not have sufficient floating point
	// accuracy to get acceptable results. For ease of rendering, the
	// storage order is counterclockwise starting from bottom left.
	struct {
		struct cache_data_coords sw;	// Southwest
		struct cache_data_coords se;	// Southeast
		struct cache_data_coords ne;	// Northeast
		struct cache_data_coords nw;	// Northwest
	} __attribute__((packed)) coords;
};

struct cache_config {

	// Total maximum number of active cache members.
	size_t capacity;

	// Callback function to call when an element is purged.
	void (* destroy) (struct cache_data *);
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
extern const struct cache_data *cache_insert (struct cache *cache, const struct cache_node *loc, struct cache_data *data);

// Retrieve data from the cache at a given level. The function returns data at
// this zoom level or lower. The #out member describes the returned node.
extern const struct cache_data *cache_search (struct cache *cache, const struct cache_node *in, struct cache_node *out);

// Cache creation/destruction.
extern void cache_destroy (struct cache *cache);
extern struct cache *cache_create (const struct cache_config *config);
