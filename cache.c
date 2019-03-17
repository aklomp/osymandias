#include <stdlib.h>
#include <stdio.h>
#include <inttypes.h>

#include "cache.h"
#include "util.h"

#define ZOOM_MAX	19

// One node in a given zoom level.
struct node {
	union cache_data data;
	uint32_t x;
	uint32_t y;
	uint32_t atime;
};

// One zoom level, containing nodes.
struct level {
	struct node *node;
	uint32_t     used;
};

// Cache descriptor.
struct cache {
	void (* destroy) (union cache_data *);
	uint32_t used;
	uint32_t capacity;
	uint32_t counter;
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

// Search in current zoom level.
static struct node *
search_level (struct cache *c, const struct cache_node *req)
{
	struct level *level = &c->level[req->zoom];

	FOREACH_NELEM (level->node, level->used, node)
		if (node->x == req->x && node->y == req->y)
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

// Remove given node.
static void
destroy (struct cache *c, struct level *l, struct node *n)
{
	c->destroy(&n->data);
	c->used--;

	// Get last node, decrement usage count:
	const struct node *last = &l->node[--l->used];

	// Copy last node over this one:
	if (n != last)
		*n = *last;
}

// Remove the node accessed longest ago across all zoom layers.
static void
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

	if (victim.n != NULL)
		destroy(c, victim.l, victim.n);
}

// Search for a matching node at given zoom level or lower.
union cache_data *
cache_search (struct cache *c, const struct cache_node *in, struct cache_node *out)
{
	struct node *node;

	if ((node = search(c, in, out)) == NULL)
		return NULL;

	node->atime = ++c->counter;
	return &node->data;
}

// Replace the data in an existing node.
static bool
replace (struct cache *c, const struct cache_node *loc, union cache_data *data)
{
	struct node *n;

	if ((n = search_level(c, loc)) == NULL)
		return false;

	c->destroy(&n->data);
	n->data  = *data;
	n->atime = ++c->counter;
	return true;
}

void
cache_insert (struct cache *c, const struct cache_node *loc, union cache_data *data)
{
	// Run basic sanity checks on the given location:
	if (valid(loc) == false) {
		c->destroy(data);
		return;
	}

	// If a node already exists at the location, reuse it:
	if (replace(c, loc, data))
		return;

	// If the list is at capacity, evict the oldest accessed node:
	if (c->used == c->capacity)
		purge_stalest(c);

	// Insert this node last:
	struct level *l = &c->level[loc->zoom];
	struct node  *n = &l->node[l->used++];

	c->used++;
	n->x = loc->x;
	n->y = loc->y;
	n->data = *data;
	n->atime = ++c->counter;
}

void
cache_destroy (struct cache *c)
{
	if (c == NULL)
		return;

	FOREACH (c->level, level) {
		FOREACH_NELEM (level->node, level->used, node) {
			c->destroy(&node->data);
		}
		free(level->node);
	}
	free(c);
}

struct cache *
cache_create (const struct cache_config *config)
{
	struct cache *c;
	uint32_t cap, max = 1;

	if ((c = calloc(1, sizeof (*c))) == NULL)
		return NULL;

	c->capacity = config->capacity;
	c->destroy  = config->destroy;

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
