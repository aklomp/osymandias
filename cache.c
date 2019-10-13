#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <inttypes.h>

#include "cache.h"
#include "util.h"

#define ZOOM_MAX	19

// One node in a given zoom level.
struct node {
	union {
		struct {
			uint32_t x;
			uint32_t y;
		} __attribute__((packed));
		uint64_t key;
	};
	uint32_t atime;
	uint32_t index;
} __attribute__((packed,aligned(16)));

// One zoom level, containing nodes.
struct level {
	struct node *node;
	uint32_t     used;
};

// Cache descriptor.
struct cache {

	// Destruction function for an entry, provided by the user.
	void (* destroy) (void *);

	// Array of cache entries, allocated at runtime.
	uint8_t *entry;

	// Number of entries in use.
	uint32_t used;

	// Total number of entries available.
	uint32_t capacity;

	// An incrementing counter which is used as a lifetime clock.
	uint32_t counter;

	// Size of an entry in bytes.
	uint32_t entrysize;

	// Array of zoom level structures for each zoom level, including zero.
	struct level level[ZOOM_MAX + 1];
};

// Check node for validity.
static bool
valid (const struct cache_node *n)
{
	if (n == NULL) {
		fprintf(stderr, "Cache error: null pointer\n");
		return false;
	}

	if (n->zoom > ZOOM_MAX) {
		fprintf(stderr, "Cache error: zoom exceeds max: "
			"(%" PRIu32 ",%" PRIu32 ")@%" PRIu32 "\n",
			n->x, n->y, n->zoom);
		return false;
	}

	const uint32_t width = UINT32_C(1) << n->zoom;

	if (n->x >= width || n->y >= width) {
		fprintf(stderr, "Cache error: coord(s) exceed max: "
			"(%" PRIu32 ",%" PRIu32 ")@%" PRIu32 "\n",
			n->x, n->y, n->zoom);
		return false;
	}

	return true;
}

// Get a pointer to a cache entry.
static inline void *
entry_ptr (struct cache *c, const uint32_t index)
{
	return c->entry + index * c->entrysize;
}

// Search in current zoom level.
static struct node *
search_level (struct cache *c, const struct cache_node *req)
{
	struct level *level = &c->level[req->zoom];

	FOREACH_NELEM (level->node, level->used, node)
		if (node->key == req->key)
			return node;

	return NULL;
}

// Search in current zoom level or lower.
static struct node *
search (struct cache *c, const struct cache_node *in, struct cache_node *out)
{
	*out = *in;

	for (;;) {
		struct node *node;

		// Return node if found at this zoom level:
		if ((node = search_level(c, out)) != NULL)
			return node;

		// Go up one zoom level if possible:
		if (cache_node_up(out) == false)
			break;
	}

	return NULL;
}

// Remove given node, return the index in the entry array that has become
// vacant. Call only when purging the stalest node.
static uint32_t
destroy (struct cache *c, struct level *l, struct node *n)
{
	uint32_t index = n->index;

	// Vacate the entry:
	c->destroy(entry_ptr(c, index));

	// Get last node, decrement usage count:
	const struct node *last = &l->node[--l->used];

	// Copy last node over this one:
	if (n != last)
		*n = *last;

	return index;
}

// Remove the node accessed longest ago across all zoom layers, return the
// index in the entry array which has been made available.
static uint32_t
purge_stalest (struct cache *c)
{
	struct {
		struct node  *n;
		struct level *l;
	} victim = { NULL, NULL };

	FOREACH (c->level, level)
		FOREACH_NELEM (level->node, level->used, node)
			if (victim.n == NULL || node->atime < victim.n->atime) {
				victim.n = node;
				victim.l = level;
			}

	return victim.n == NULL ? 0 : destroy(c, victim.l, victim.n);
}

// Search for a matching node at given zoom level or lower.
void *
cache_search (struct cache *c, const struct cache_node *in, struct cache_node *out)
{
	struct node *node;

	if ((node = search(c, in, out)) == NULL)
		return NULL;

	node->atime = ++c->counter;
	return entry_ptr(c, node->index);
}

// Replace the data in an existing node.
static const struct node *
replace (struct cache *c, const struct cache_node *loc, void *data)
{
	struct node *n;

	if ((n = search_level(c, loc)) == NULL)
		return NULL;

	c->destroy(entry_ptr(c, n->index));
	memcpy(entry_ptr(c, n->index), data, c->entrysize);

	n->atime = ++c->counter;
	return n;
}

void *
cache_insert (struct cache *c, const struct cache_node *loc, void *data)
{
	uint32_t index;

	// Run basic sanity checks on the given location:
	if (valid(loc) == false) {
		c->destroy(data);
		return NULL;
	}

	// If a node already exists at the location, reuse it:
	const struct node *reused;
	if ((reused = replace(c, loc, data)) != NULL)
		return entry_ptr(c, reused->index);

	// If the list is at capacity, evict the oldest accessed node:
	index = c->used == c->capacity ? purge_stalest(c) : c->used++;

	// Insert this node last:
	struct level *l = &c->level[loc->zoom];
	struct node  *n = &l->node[l->used++];

	// Insert the data into the entry array:
	memcpy(entry_ptr(c, index), data, c->entrysize);

	n->key   = loc->key;
	n->atime = ++c->counter;
	n->index = index;
	return entry_ptr(c, n->index);
}

void
cache_destroy (struct cache *c)
{
	if (c == NULL)
		return;

	FOREACH (c->level, level)
		free(level->node);

	for (size_t i = 0; i < c->used; i++)
		c->destroy(entry_ptr(c, i));

	free(c->entry);
	free(c);
}

struct cache *
cache_create (const struct cache_config *config)
{
	struct cache *c;
	uint32_t cap, max = 1;

	if ((c = calloc(1, sizeof (*c))) == NULL)
		return NULL;

	c->capacity  = config->capacity;
	c->destroy   = config->destroy;
	c->entrysize = config->entrysize;

	// Allocate memory for the entries:
	if ((c->entry = malloc(config->capacity * config->entrysize)) == NULL) {
		cache_destroy(c);
		return NULL;
	}

	// Allocate memory for all node arrays:
	for (size_t z = 0; z < NELEM(c->level); z++) {
		struct level *level = &c->level[z];

		// Don't allocate more slots than there are tiles in the level.
		// Calculate this carefully to not overflow the 32-bit int:
		if (max < c->capacity) {
			cap = max;
			max <<= 2;
		} else
			cap = c->capacity;

		if ((level->node = malloc(cap * sizeof (struct node))) == NULL) {
			cache_destroy(c);
			return NULL;
		}
	}

	return c;
}
