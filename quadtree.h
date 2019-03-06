#pragma once

#include <stdbool.h>

struct quadtree;

struct quadtree_req {
	int zoom;			// Desired zoom level for tile
	int x;				// Queried tile coordinates, in desired zoom level
	int y;
	int world_zoom;			// Current world zoom level
	const struct coords *center;	// Current center in tile coordinates

	void *found_data;	// Data pointer found
	int found_zoom;		// Zoom level which we could provide
	int found_x;		// The x coord of the tile in that zoom level
	int found_y;		// The y coord of the tile in that zoom level
};

extern struct quadtree * quadtree_create (int capacity, void *(*callback_procure)(struct quadtree_req *req), void (*callback_destroy)(void *data));
extern bool quadtree_request     (struct quadtree *t, struct quadtree_req *req);
extern bool quadtree_data_insert (struct quadtree *t, struct quadtree_req *req, void *data);
extern void quadtree_destroy     (struct quadtree **t);
