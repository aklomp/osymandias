#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <math.h>

#include "vector.h"
#include "vector2d.h"
#include "camera.h"
#include "tile2d.h"
#include "quadtree.h"

struct node {
	struct node *parent;
	struct node *q[2][2];	// Child quadrants
	void *data;		// This node's data
	int zoom;
	int x;
	int y;
	int viewzoom_min;
	int viewzoom_max;
	int stamp;
};

struct quadtree {
	struct node *root;
	int num_nodes;
	int num_data;
	int capacity;
	void *(*tile_procure)(struct quadtree_req *req);
	void  (*tile_destroy)(void *data);

	// Used by prune routine:
	int world_zoom;
	float cx;
	float cy;
	int stamp;
};

static struct node *
node_create (struct quadtree *t, struct node *parent, int x, int y)
{
	struct node *n;

	if ((n = malloc(sizeof(*n))) == NULL) {
		return NULL;
	}
	n->x = x;
	n->y = y;
	n->data = NULL;
	n->parent = parent;
	n->stamp = -1;
	n->zoom = (parent) ? parent->zoom + 1 : 0;
	n->q[0][0] = n->q[0][1] = n->q[1][0] = n->q[1][1] = NULL;
	t->num_nodes++;
	return n;
}

static inline void
node_data_destroy (struct quadtree *t, struct node *n)
{
	if (n->data != NULL) {
		t->tile_destroy(n->data);
		t->num_data--;
	}
	n->data = NULL;
}

static void
node_destroy_recursive (struct quadtree *t, struct node *n)
{
	// Deletes this node and all of its children.
	// Updating the parent node is the responsibility of the caller.

	if (n == NULL) return;

	node_data_destroy(t, n);

	// Unroll loop, save a stack variable:
	if (n->q[0][0]) node_destroy_recursive(t, n->q[0][0]);
	if (n->q[0][1]) node_destroy_recursive(t, n->q[0][1]);
	if (n->q[1][0]) node_destroy_recursive(t, n->q[1][0]);
	if (n->q[1][1]) node_destroy_recursive(t, n->q[1][1]);

	free(n);

	t->num_nodes--;
}

static void
purge_node_if_obsolete (struct quadtree *t, struct node *n)
{
	// A node is obsolete if it has four quad children which all have data.
	// We then don't need the data at this zoom.
	if (n == NULL || n->parent == NULL) {
		return;
	}
	// If not all quads are there, can't purge node;
	// if not all quads have data, this node is not yet obsolete:
	if (!(n->q[0][0] && n->q[0][0]->data
	   && n->q[0][1] && n->q[0][1]->data
	   && n->q[1][0] && n->q[1][0]->data
	   && n->q[1][1] && n->q[1][1]->data)) {
		return;
	}
	// Recurse on the parent; do it here because if we delete this node's
	// data first, the parent will never have four quads with data (this
	// child here will be bare):
	purge_node_if_obsolete(t, n->parent);

	// We found that this node's data can be replaced by the four child
	// quads, so delete the data to leave this node empty (we know it has children):
	node_data_destroy(t, n);
}

static void
node_destroy_self (struct quadtree *t, struct node *n)
{
	if (n == NULL) return;

	int qx = n->x & 1;
	int qy = n->y & 1;
	struct node *parent = n->parent;

	node_destroy_recursive(t, n);

	if (parent) parent->q[qx][qy] = NULL;
}

static void
purge_below (struct quadtree *t, struct node *n, int zoom, int prune_target)
{
	// If at given zoom, leave this node alone but destroy its children:
	if (n->zoom > zoom) {
		node_destroy_self(t, n);
		return;
	}
	if (n->q[0][0]) { purge_below(t, n->q[0][0], zoom, prune_target); if (t->num_data <= prune_target) return; }
	if (n->q[0][1]) { purge_below(t, n->q[0][1], zoom, prune_target); if (t->num_data <= prune_target) return; }
	if (n->q[1][0]) { purge_below(t, n->q[1][0], zoom, prune_target); if (t->num_data <= prune_target) return; }
	if (n->q[1][1]) { purge_below(t, n->q[1][1], zoom, prune_target); }
}

static void
node_update_viewzoom (struct quadtree *t, struct node *n)
{
	// If tree and node have the same "stamp", we already did this
	// expensive calculation for this round:
	if (n->stamp == t->stamp) {
		return;
	}
	// Calculate zoom levels for the four quads of this node:
	int zoomdiff = t->world_zoom - n->zoom;

	// Get max zoom with this special function, that not only takes into
	// account the corner tiles, but also the tile directly under the
	// cursor:
	n->viewzoom_max = tile2d_get_max_zoom
	(
		// Coordinate of this tile at current world zoom:
		n->x << zoomdiff,
		n->y << zoomdiff,

		// Width of the tile at current world zoom:
		1 << zoomdiff,

		// Current view data:
		t->cx, t->cy, t->world_zoom
	);
	// Get min zoom by getting the corner zooms of the tile:
	vec4i zooms = tile2d_get_corner_zooms
	(
		// Coordinate of this tile at current world zoom:
		n->x << zoomdiff,
		n->y << zoomdiff,

		// Width of the tile at current world zoom:
		1 << zoomdiff,

		// Current view data:
		t->cx, t->cy, t->world_zoom
	);
	int m1 = (zooms[0] < zooms[1]) ? zooms[0] : zooms[1];
	int m2 = (zooms[2] < zooms[3]) ? zooms[2] : zooms[3];
	n->viewzoom_min = (m1 < m2) ? m1 : m2;

	n->stamp = t->stamp;
}

static void
purge_below_visibility (struct quadtree *t, struct node *n, int prune_target)
{
	node_update_viewzoom(t, n);

	// Should this tile already not be visible? Destroy children:
	if (n->zoom > n->viewzoom_max) {
		node_destroy_self(t, n);
		return;
	}
	if (n->q[0][0]) { purge_below_visibility(t, n->q[0][0], prune_target); if (t->num_data <= prune_target) return; }
	if (n->q[0][1]) { purge_below_visibility(t, n->q[0][1], prune_target); if (t->num_data <= prune_target) return; }
	if (n->q[1][0]) { purge_below_visibility(t, n->q[1][0], prune_target); if (t->num_data <= prune_target) return; }
	if (n->q[1][1]) { purge_below_visibility(t, n->q[1][1], prune_target); }
}

static void
quadtree_prune (struct quadtree *t, struct node *n, int prune_target)
{
	// Purge all tiles below current zoom level:
	purge_below(t, t->root, t->world_zoom, prune_target);
	if (t->num_data <= prune_target) return;

	// Calculate visible zoom level (a function of distance) for
	// all tiles, delete those that are "too high-res":
	purge_below_visibility(t, n, prune_target);
}

static struct node *
node_find_parent_for (const struct node *a, int zoom, int x, int y)
{
	struct node *n = (struct node *)a;

	// The parent is always at same or lower zoom level than given:
	while (n->zoom > zoom) {
		n = n->parent;
	}
	x >>= zoom - n->zoom;
	y >>= zoom - n->zoom;
	int nx = n->x;
	int ny = n->y;

	while ((x ^ nx) || (y ^ ny)) {
		x >>= 1; y >>= 1;
		nx >>= 1; ny >>= 1;
		n = n->parent;
	}
	return n;
}

struct quadtree *
quadtree_create (int capacity,
	void *(*callback_procure)(struct quadtree_req *req),
	void  (*callback_destroy)(void *data))
{
	struct quadtree *t;

	if ((t = malloc(sizeof(*t))) == NULL) {
		return NULL;
	}
	t->num_data = 0;
	t->num_nodes = 0;
	t->stamp = 0;
	t->capacity = capacity;
	t->tile_procure = callback_procure;
	t->tile_destroy = callback_destroy;

	// Create empty root node:
	if ((t->root = node_create(t, NULL, 0, 0)) == NULL) {
		free(t);
		return NULL;
	}
	return t;
}

void
quadtree_destroy (struct quadtree **t)
{
	node_destroy_recursive(*t, (*t)->root);

	free(*t);
	*t = NULL;
}

static bool
_quadtree_request (struct quadtree *t, struct quadtree_req *req, struct node *n)
{
	// Node is too low in the hierarchy? Find common parent, start there:
	if (req->zoom < n->zoom || (req->zoom == n->zoom && (req->x != n->x || req->y != n->y))) {
		return _quadtree_request(t, req, node_find_parent_for(n, req->zoom, req->x, req->y));
	}
	// Not at the right node yet? Go deeper:
	else if (req->zoom > n->zoom)
	{
		int qx = (req->x >> (req->zoom - n->zoom - 1)) & 1;
		int qy = (req->y >> (req->zoom - n->zoom - 1)) & 1;
		struct node *q = n->q[qx][qy];

		// Successfully got the data from a lower level:
		if (q != NULL && _quadtree_request(t, req, q)) {
			return true;
		}
	}
	// Otherwise, use this node's data if present:
	if (n->data == NULL || n->zoom < req->zoom)
	{
		// Last effort: procure data:
		if (t->tile_procure && (req->found_data = t->tile_procure(req)) != NULL) {
			if (quadtree_data_insert(t, req, req->found_data)) {
				req->found_x = req->x;
				req->found_y = req->y;
				req->found_zoom = req->zoom;
				return true;
			}
			t->tile_destroy(req->found_data);
			return false;
		}
		// If this tile has no data either, give up:
		if (n->data == NULL) {
			return false;
		}
	}
	req->found_x = n->x;
	req->found_y = n->y;
	req->found_zoom = n->zoom;
	req->found_data = n->data;
	return true;
}

bool
quadtree_request (struct quadtree *t, struct quadtree_req *req)
{
	return _quadtree_request(t, req, t->root);
}

static inline void
node_data_update (struct quadtree *t, struct node *n, void *data)
{
	// Delete any existing data at this location:
	if (n->data != NULL) {
		node_data_destroy(t, n);
	}
	if ((n->data = data) != NULL)
	{
		// Maybe the parent is now made obsolete by this new data:
		// TODO: we don't always want to delete the parent straight away.
		purge_node_if_obsolete(t, n->parent);

		// Went over capacity? Purge:
		if (++t->num_data > t->capacity)
		{
			// Amount of filled nodes to purge down to:
			quadtree_prune(t, t->root, (t->capacity * 3) / 4);
		}
	}
}

bool
quadtree_data_insert (struct quadtree *t, struct quadtree_req *req, void *data)
{
	struct node *n = t->root;

	// Reach node from root, create nonexistent nodes:
	while (n->zoom < req->zoom)
	{
		int qx = (req->x >> (req->zoom - n->zoom - 1)) & 1;
		int qy = (req->y >> (req->zoom - n->zoom - 1)) & 1;

		// Already have a node in this slot?
		if (n->q[qx][qy] != NULL) {
			n = n->q[qx][qy];
			continue;
		}
		// Else create the node in question:
		struct node *p;

		if ((p = node_create(t, n,
			req->x >> (req->zoom - n->zoom - 1),
			req->y >> (req->zoom - n->zoom - 1))) == NULL) {
			return false;
		}
		n = n->q[qx][qy] = p;
	}
	// If this is the actual node, park the data;
	// set 'world parameters' in object first, in case we need to do pruning:
	t->world_zoom = req->world_zoom;
	t->cx = req->cx;
	t->cy = req->cy;
	t->stamp++;
	node_data_update(t, n, data);

	return true;
}
