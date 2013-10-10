#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "world.h"
#include "viewport.h"
#include "vector.h"
#include "vector2d.h"
#include "camera.h"
#include "tile2d.h"

#define MEMPOOL_BLOCK_SIZE 100

struct tile {
	int x;
	int y;
	int wd;
	int ht;
	int zoom;
	struct tile *prev;
	struct tile *next;
};

struct mempool_block {
	struct tile tile[MEMPOOL_BLOCK_SIZE];
	struct mempool_block *next;
};

static int world_size;
static int world_zoom;
static int tile_top;
static int tile_left;
static int tile_right;
static int tile_bottom;
static float center_x;
static float center_y;

static struct tile *drawlist = NULL;
static struct tile *drawlist_tail = NULL;
static struct tile *drawlist_iter = NULL;

static struct mempool_block *mempool = NULL;
static struct mempool_block *mempool_tail = NULL;
static int mempool_idx = 0;

static int tile_get_zoom (int tile_x, int tile_y);
static void reduce_block (int real_x, int real_y, int tilesize, int maxzoom);
static void optimize_block (int x, int y, int relzoom);

static struct tile *
mempool_request_tile (void)
{
	// This mempool just grows and grows till the program ends. Since the
	// number of tiles on screen is sort of bounded, that doesn't cause a
	// problem. At each recalc, we reuse the existing pool.

	// Reached end of current block, or no blocks yet:
	if (mempool_idx == MEMPOOL_BLOCK_SIZE || mempool_tail == NULL)
	{
		// Move to next block if available:
		if (mempool_tail != NULL && mempool_tail->next != NULL) {
			mempool_tail = mempool_tail->next;
			mempool_idx = 0;
			goto out;
		}
		// Else, allocate new block, hang in list:
		struct mempool_block *p;

		if ((p = malloc(sizeof(*p))) == NULL) {
			return NULL;
		}
		p->next = NULL;

		// Is this the first block?
		// Check of mempool_tail is to silence 'scan-build':
		if (mempool == NULL || mempool_tail == NULL) {
			mempool = p;
		}
		else {
			mempool_tail->next = p;
		}
		mempool_tail = p;
		mempool_idx = 0;
	}
out:	return &mempool_tail->tile[mempool_idx++];
}

static void
mempool_reset (void)
{
	// Reset tail to start:
	mempool_tail = mempool;
	mempool_idx = 0;
}

static void
mempool_destroy (void)
{
	struct mempool_block *p = mempool;
	struct mempool_block *q;

	while (p) {
		q = p->next;
		free(p);
		p = q;
	}
}

static bool
drawlist_add (const struct tile *const tile)
{
	struct tile *t;

	if ((t = mempool_request_tile()) == NULL) {
		return false;
	}
	memcpy(t, tile, sizeof(*t));

	t->prev = NULL;
	t->next = NULL;

	if (drawlist == NULL) {
		drawlist = drawlist_tail = t;
	}
	else {
		t->prev = drawlist_tail;
		drawlist_tail->next = t;
		drawlist_tail = t;
	}
	return true;
}

static void
drawlist_detach (struct tile *t)
{
	// Detach and free a tile from the drawlist:
	if (t->prev) {
		t->prev->next = t->next;
	}
	else {
		drawlist = t->next;
	}
	if (t->next) {
		t->next->prev = t->prev;
	}
	else {
		drawlist_tail = t->prev;
	}
}

static void
tile_farthest_get_zoom (int *zoom)
{
	// Find the tile farthest away from the viewport.
	// This is the smallest tile rendered, and hence must be a tile at the
	// lowest zoom level. It must be one of the corner tiles of the
	// bounding box. We cheat slightly by using the distance from the
	// center point, not the actual pixel dimensions of the tile.

	vec4i vz = tile2d_get_corner_zooms_abs(tile_left, tile_top, tile_right, tile_bottom, center_x, center_y, world_zoom);

	int min1 = (vz[0] < vz[1]) ? vz[0] : vz[1];
	int min2 = (vz[2] < vz[3]) ? vz[2] : vz[3];
	*zoom = (min1 < min2) ? min1 : min2;
}

void
tilepicker_recalc (void)
{
	int lowzoom;

	// This function first calculates values and puts them in global
	// variables, so that the other routines in this module can use them.
	// I'm normally not a fan of globals, but this does make the code
	// simpler.
	world_zoom = world_get_zoom();
	world_size = world_get_size();

	tile_top = viewport_get_tile_top();
	tile_left = viewport_get_tile_left();
	tile_right = viewport_get_tile_right();
	tile_bottom = viewport_get_tile_bottom();

	// Convert center_y to *tile* coordinates; much easier to work with:
	center_x = viewport_get_center_x();
	center_y = world_size - viewport_get_center_y();

	// Reset drawlist pointers. The drawlist consists of tiles allocated in
	// the mempool that point to each other; the drawlist itself does not
	// involve allocations.
	drawlist = drawlist_tail = NULL;

	// Reset tile mempool pointers. For this round, start giving out tiles
	// from the top of the mempool again.
	mempool_reset();

	tile_farthest_get_zoom(&lowzoom);

	// Start with biggest block size in world; recursively cut these into smaller blocks:
	int zoomdiff = world_zoom - lowzoom;
	int tilesize = 1 << zoomdiff;
	for (int tile_x = tile_left & ~(tilesize - 1); tile_x <= tile_right; tile_x += tilesize) {
		for (int tile_y = tile_top & ~(tilesize - 1); tile_y <= tile_bottom; tile_y += tilesize) {
			reduce_block(tile_x, tile_y, tilesize, lowzoom);
			optimize_block(tile_x, tile_y, zoomdiff);
		}
	}
}

static bool
tile_is_visible (const struct tile *const tile)
{
	// Ensure that the edges rotate clockwise in the "up" direction:
	return camera_visible_quad(
		tile_to_world((vec4f){ tile->x,            tile->y,            0, 0 }, center_x, center_y),
		tile_to_world((vec4f){ tile->x + tile->wd, tile->y,            0, 0 }, center_x, center_y),
		tile_to_world((vec4f){ tile->x + tile->wd, tile->y + tile->ht, 0, 0 }, center_x, center_y),
		tile_to_world((vec4f){ tile->x,            tile->y + tile->ht, 0, 0 }, center_x, center_y)
	);
}

static void
reduce_block (int x, int y, int size, int maxzoom)
{
	struct tile tile = { .x = x, .y = y, .wd = size, .ht = size };

	// Not even visible? Give up:
	if (!tile_is_visible(&tile)) {
		return;
	}
	// This "quad" is a single tile:
	if (size == 1) {
		tile.zoom = tile_get_zoom(x, y);
		drawlist_add(&tile);
		return;
	}
	int halfsize = size / 2;

	// Sign test of the four corner points: all x's or all y's should lie
	// to one side of the center, else split up in four quadrants of the same zoom level:
	bool signs_x_equal = ((x < center_x) == ((x + size) < center_x));
	bool signs_y_equal = ((y < center_y) == ((y + size) < center_y));

	if (!(signs_x_equal || signs_y_equal)) {
		reduce_block(x,            y,            halfsize, maxzoom + 1);
		reduce_block(x + halfsize, y,            halfsize, maxzoom + 1);
		reduce_block(x + halfsize, y + halfsize, halfsize, maxzoom + 1);
		reduce_block(x,            y + halfsize, halfsize, maxzoom + 1);
		return;
	}
	// Is all of this quad at the proper zoom level? Cut it into four quadrants
	// with 9 corner points:
	//
	//  0--1--2
	//  | 0| 1|
	//  7--8--3
	//  | 3| 2|
	//  6--5--4
	//
	// These are the corner points in tile coordinates, numbered as above:
	int corner[9][2] = {
		{ x,            y            },	// 0
		{ x + halfsize, y            },	// 1
		{ x + size,     y            },	// 2
		{ x + size,     y + halfsize },	// 3
		{ x + size,     y + size     },	// 4
		{ x + halfsize, y + size     },	// 5
		{ x,            y + size     },	// 6
		{ x,            y + halfsize },	// 7
		{ x + halfsize, y + halfsize }	// 8
	};
	// These are our quads by corner point:
	int quad[4][4] = {
		{ 0, 1, 8, 7 },
		{ 1, 2, 3, 8 },
		{ 8, 3, 4, 5 },
		{ 7, 8, 5, 6 }
	};
	// Get the zoom levels for all four quads:
	int i;
	vec4i z[4];

	if (halfsize == 1) {
		for (int q = 0; q < 4; q++) {
			z[q][0] = z[q][1] = z[q][2] = z[q][3] = tile_get_zoom(corner[quad[q][0]][0], corner[quad[q][0]][1]);
		}
	}
	else {
		for (int q = 0; q < 4; q++) {
			z[q] = tile2d_get_corner_zooms(corner[quad[q][0]][0], corner[quad[q][0]][1], halfsize, center_x, center_y, world_zoom);
		}
	}
	// Does the quadrant in question have a uniform zoom level?
	int quad_consistent[4];
	for (int q = 0; q < 4; q++) {
		for (i = 1; i < 4; i++) {
			if (z[q][i] != z[q][i - 1]) {
				break;
			}
		}
		// Not only must the four corner points be consistent, but so also must the closest tile:
		// TODO: check if this is actually necessary
		quad_consistent[q] = (i == 4 && (halfsize <= 2 || tile2d_get_max_zoom(corner[quad[q][0]][0], corner[quad[q][0]][1], halfsize, center_x, center_y, world_zoom) == z[q][0]));
	}
	// If all zoom levels are equal, add the whole block:
	// but only if the tile size is smaller or equal than the zoom max tile size:
	if (quad_consistent[0] && quad_consistent[1] && quad_consistent[2] && quad_consistent[3]
	 && z[0][0] == z[1][0] && z[1][0] == z[2][0] && z[2][0] == z[3][0]
	 && z[0][0] <= maxzoom) {
		tile.x = x;
		tile.y = y;
		tile.wd = size;
		tile.ht = size;
		tile.zoom = z[0][0];
		drawlist_add(&tile);
		return;
	}
	int processed[4] = { 0, 0, 0, 0 };

	// If top two quadrants have correct zoom level, add them as block:
	if (quad_consistent[0] && quad_consistent[1] && z[0][0] == z[1][0] && z[0][0] <= maxzoom) {
		tile.x = x;
		tile.y = y;
		tile.wd = size;
		tile.ht = halfsize;
		if (tile_is_visible(&tile)) {
			tile.zoom = z[0][0];
			drawlist_add(&tile);
			processed[0] = processed[1] = 1;
		}
	}
	// If bottom two quadrants have correct zoom level, add them as block:
	else if (quad_consistent[3] && quad_consistent[2] && z[3][0] == z[2][0] && z[2][0] <= maxzoom) {
		tile.x = x;
		tile.y = y + halfsize;
		tile.wd = size;
		tile.ht = halfsize;
		if (tile_is_visible(&tile)) {
			tile.zoom = z[3][0];
			drawlist_add(&tile);
			processed[3] = processed[2] = 1;
		}
	}
	// If left two quadrants have correct zoom level, add them as block:
	if (!processed[0] && !processed[3] && quad_consistent[0] && quad_consistent[3] && z[0][0] == z[3][0] && z[0][0] <= maxzoom) {
		tile.x = x;
		tile.y = y;
		tile.wd = halfsize;
		tile.ht = size;
		if (tile_is_visible(&tile)) {
			tile.zoom = z[0][0];
			drawlist_add(&tile);
			processed[0] = processed[3] = 1;
		}
	}
	// If right two quadrants have correct zoom level, add them as block:
	if (!processed[1] && !processed[2] && quad_consistent[1] && quad_consistent[2] && z[1][0] == z[2][0] && z[1][0] <= maxzoom) {
		tile.x = x + halfsize;
		tile.y = y;
		tile.wd = halfsize;
		tile.ht = size;
		if (tile_is_visible(&tile)) {
			tile.zoom = z[1][0];
			drawlist_add(&tile);
			processed[1] = processed[2] = 1;
		}
	}
	// Check individual quadrants:
	for (int q = 0; q < 4; q++) {
		if (processed[q]) {
			continue;
		}
		int corner_x = corner[quad[q][0]][0];
		int corner_y = corner[quad[q][0]][1];
		if (quad_consistent[q] && z[q][0] <= maxzoom) {
			tile.x = corner_x;
			tile.y = corner_y;
			tile.wd = halfsize;
			tile.ht = halfsize;
			if (tile_is_visible(&tile)) {
				tile.zoom = z[q][0];
				drawlist_add(&tile);
				continue;
			}
		}
		reduce_block(corner_x, corner_y, halfsize, maxzoom + 1);
	}
}

static void
optimize_block (int x, int y, int relzoom)
{
	int sz = (1 << relzoom);
	int z = world_zoom - relzoom;
	int have_smaller = 0;
	struct tile *tile1, *tile2;

reset:	for (tile1 = drawlist; tile1 != NULL; tile1 = tile1->next)
	{
		// Check if tile is part of this block:
		if (tile1->x < x || tile1->x + tile1->wd > x + sz
		 || tile1->y < y || tile1->y + tile1->ht > y + sz) {
			continue;
		}
		// Higher zoom level found, register the fact:
		if (tile1->zoom > z) {
			have_smaller = 1;
		}
		// If not at the zoom level we're looking at, ignore:
		if (tile1->zoom != z) {
			continue;
		}
		// Find second tile to compare with:
		for (tile2 = drawlist; tile2 != NULL; tile2 = tile2->next) {
			if (tile1 == tile2 || tile2->zoom != z
			 || tile2->x < x || tile2->x + tile2->wd > x + sz
			 || tile2->y < y || tile2->y + tile2->ht > y + sz) {
				continue;
			}
			// Do tiles share width and are they vertical neighbours?
			if (tile1->wd == tile2->wd && tile1->x == tile2->x) {
				if (tile1->y + tile1->ht == tile2->y) {
					tile1->ht += tile2->ht;
					drawlist_detach(tile2);
					goto reset;
				}
				if (tile2->y + tile2->ht == tile1->y) {
					tile2->ht += tile1->ht;
					drawlist_detach(tile1);
					goto reset;
				}
			}
			// Do tiles share height and are they horizontal neighbours?
			if (tile1->ht == tile2->ht && tile1->y == tile2->y) {
				if (tile1->x + tile1->wd == tile2->x) {
					tile1->wd += tile2->wd;
					drawlist_detach(tile2);
					goto reset;
				}
				if (tile2->x + tile2->wd == tile1->x) {
					tile2->wd += tile1->wd;
					drawlist_detach(tile1);
					goto reset;
				}
			}
		}
	}
	// If we reach this, could not combine; if we noted smaller tiles,
	// divide blocks in four quads, move on to next zoom level:
	if (!have_smaller || (sz /= 2) == 0) {
		return;
	}
	optimize_block(x,      y,      relzoom - 1);
	optimize_block(x + sz, y,      relzoom - 1);
	optimize_block(x + sz, y + sz, relzoom - 1);
	optimize_block(x,      y + sz, relzoom - 1);
}

static int
tile_get_zoom (int tile_x, int tile_y)
{
	// Shortcut: if the tilt is exactly 0.0, always use world zoom:
	if (!camera_is_tilted()) {
		return world_zoom;
	}
	return tile2d_get_zoom(tile_x, tile_y, center_x, center_y, world_zoom);
}

bool
tilepicker_next (int *x, int *y, int *wd, int *ht, int *zoom)
{
	// Returns true or false depending on whether a tile is available and
	// returned in the pointer arguments.
	if (drawlist_iter == NULL) {
		return false;
	}
	*x = drawlist_iter->x;
	*y = drawlist_iter->y;
	*wd = drawlist_iter->wd;
	*ht = drawlist_iter->ht;
	*zoom = drawlist_iter->zoom;

	drawlist_iter = drawlist_iter->next;
	return true;
}

bool
tilepicker_first (int *x, int *y, int *wd, int *ht, int *zoom)
{
	// Reset iterator:
	drawlist_iter = drawlist;

	return tilepicker_next(x, y, wd, ht, zoom);
}

void
tilepicker_destroy (void)
{
	mempool_destroy();
}
