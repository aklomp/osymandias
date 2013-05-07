#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>

#include "quadtree.h"

struct node {
	struct node *parent;
	struct node *q[2][2];	// Child quadrants
	void *data;		// This node's data
	int zoom;
	int x;
	int y;
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
		if (t->tile_procure && (req->found_data = t->tile_procure(req)) != NULL
		  && quadtree_data_insert(t, req, req->found_data)) {
			req->found_x = req->x;
			req->found_y = req->y;
			req->found_zoom = req->zoom;
			return true;
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
	if ((n->data = data) != NULL) {
		t->num_data++;

		// Maybe the parent is now made obsolete by this new data:
		// TODO: we don't always want to delete the parent straight away.
		purge_node_if_obsolete(t, n->parent);
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
	node_data_update(t, n, data);

	return true;
}
