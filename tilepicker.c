#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <vec/vec.h>

#include "worlds.h"
#include "viewport.h"
#include "camera.h"
#include "tile2d.h"
#include "tilepicker.h"

#define MEMPOOL_BLOCK_SIZE 100

// The minimal number of subdivisions the globe sphere should have at the
// lowest zoom levels, as a power of two. At zoom level 0, the world is a
// single tile in size. In order to keep the spherical appearance at that
// level, and to make the visibility logic work, we split the sphere into
// (2 << MIN_SUBDIVISIONS) subdivisions.
#define MIN_SUBDIVISIONS 4

struct vertex {
	int zoom;
	struct {
		float x;
		float y;
	} tile;
	union vec coords;
	union vec normal;
};

// A tile is split up into four quadrants:
//
//   +---+---+
//   | 0 | 1 |
//   +---+---+
//   | 3 | 2 |
//   +---+---+
//
struct quad {
	bool used;			// Quad has been output already
	struct vertex *vertex[4];	// Corner vertices
};

// Also define four rectangles which are made of up to four quads each:
//
//   0-------2   0-------2   0---1---2   0---1---2
//   |       |   |   1   |   |   |   |   | 5 | 6 |
//   |   0   |   7-------3   | 3 | 4 |   7---8---3
//   |       |   |   2   |   |   |   |   | 8 | 7 |
//   6-------4   6-------4   6---5---4   6---5---4
//
struct rect {
	float wd;
	float ht;
	struct quad *quad[4];		// Component quads
	struct vertex *vertex[4];	// Corner vertices
};

struct tile {
	float x;
	float y;
	float wd;
	float ht;
	int zoom;
	struct vertex vertex[4];
	struct tile *prev;
	struct tile *next;
};

struct mempool_block {
	struct tile tile[MEMPOOL_BLOCK_SIZE];
	struct mempool_block *next;
};

static int world_size;
static int world_zoom;

static struct tile *drawlist = NULL;
static struct tile *drawlist_tail = NULL;
static struct tile *drawlist_iter = NULL;

static struct mempool_block *mempool = NULL;
static struct mempool_block *mempool_tail = NULL;
static int mempool_idx = 0;

static void reduce_block (struct tile *tile, int maxzoom, float minsize);
static void optimize_block (int x, int y, int relzoom);

static inline union vec
vertex_all_x (const struct vertex *v)
{
	return vec(v[0].coords.x, v[1].coords.x, v[2].coords.x, v[3].coords.x);
}

static inline union vec
vertex_all_y (const struct vertex *v)
{
	return vec(v[0].coords.y, v[1].coords.y, v[2].coords.y, v[3].coords.y);
}

static inline union vec
vertex_all_z (const struct vertex *v)
{
	return vec(v[0].coords.z, v[1].coords.z, v[2].coords.z, v[3].coords.z);
}

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
		// Else, allocate new aligned block, hang in list:
		struct mempool_block *p;

		if (posix_memalign((void **)&p, 16, sizeof(*p)))
			return NULL;

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

static inline void
project (struct vertex *v)
{
	world_project_tile(&v->coords, &v->normal, v->tile.x, v->tile.y);
}

static bool
tile_is_visible (struct tile *const tile)
{
	const union vec *coords[4] = {
		&tile->vertex[0].coords,
		&tile->vertex[1].coords,
		&tile->vertex[2].coords,
		&tile->vertex[3].coords,
	};

	return camera_visible_quad(coords);
}

static bool
tile_edges_agree (struct tile *const tile)
{
	const int zoom_highest = zoom_edges_highest(
		world_zoom,
		vertex_all_x(tile->vertex),
		vertex_all_y(tile->vertex),
		vertex_all_z(tile->vertex));

	return (tile->zoom == zoom_highest);
}

static inline bool
rect_has_single_zoomlevel (const struct rect *rect)
{
	// Check all zooms against this one:
	int baseline = rect->quad[0]->vertex[0]->zoom;

	for (int i = 0; i < 4; i++) {
		struct quad *quad = rect->quad[i];

		if (quad == NULL)
			break;

		for (int j = 0; j < 4; j++)
			if (quad->vertex[j]->zoom != baseline)
				return false;
	}

	return true;
}

static inline bool
rect_quads_unused (const struct rect *rect)
{
	for (int i = 0; i < 4; i++) {
		struct quad *quad = rect->quad[i];

		if (quad == NULL)
			break;

		if (quad->used)
			return false;
	}

	return true;
}

static inline void
rect_quads_mark_used (const struct rect *rect)
{
	for (int i = 0; i < 4; i++) {
		struct quad *quad = rect->quad[i];

		if (quad == NULL)
			return;

		quad->used = true;
	}
}

void
tilepicker_recalc (void)
{
	// This function first calculates values and puts them in global
	// variables, so that the other routines in this module can use them.
	// I'm normally not a fan of globals, but this does make the code
	// simpler.
	world_zoom = world_get_zoom();
	world_size = world_get_size();

	// Reset drawlist pointers. The drawlist consists of tiles allocated in
	// the mempool that point to each other; the drawlist itself does not
	// involve allocations.
	drawlist = drawlist_tail = NULL;

	// Reset tile mempool pointers. For this round, start giving out tiles
	// from the top of the mempool again.
	mempool_reset();

	// Determine the starting zoom level. In the planar world, we start at
	// zoom level 0. In the spherical world, we want to keep the spherical
	// character, so we start at a higher zoom level for a rounder globe:
	int zoom_start = (world_get() == WORLD_PLANAR) ? 0 : MIN_SUBDIVISIONS;

	int minsize = (world_zoom > MIN_SUBDIVISIONS) ? 0 : (world_zoom - MIN_SUBDIVISIONS);

	// Loop over all tiles of the world at zoom_start:
	for (int tile_y = 0; tile_y < (1 << zoom_start); tile_y++) {
		for (int tile_x = 0; tile_x < (1 << zoom_start); tile_x++) {

			// TODO: hoist invariants out of loop
			float x = ldexpf(tile_x, world_zoom - zoom_start);
			float y = ldexpf(tile_y, world_zoom - zoom_start);
			float sz = ldexpf(1.0f, world_zoom - zoom_start);

			struct tile tile = {
				.x = x,
				.y = y,
				.wd = sz,
				.ht = sz,
				.vertex = {
					{ .tile = { x,      y,     } },
					{ .tile = { x + sz, y,     } },
					{ .tile = { x + sz, y + sz } },
					{ .tile = { x,      y + sz } },
				}
			};

			// Project vertices, calculate distance (zoom):
			for (int i = 0; i < 4; i++) {
				struct vertex *vertex = &tile.vertex[i];
				project(vertex);
				vertex->zoom = zoom_point(world_zoom, &vertex->coords);
			}

			// Recursively split this block:
			reduce_block(&tile, 0, ldexpf(1.0f, minsize));

			// Merge adjacent blocks with same zoom level:
			// Only relevant in the planar world: sphere must remain round:
			if (world_get() == WORLD_PLANAR)
				optimize_block(tile_x, tile_y, world_zoom);
		}
	}
}

static void
reduce_block (struct tile *tile, int maxzoom, float minsize)
{
	// Caller passes us a struct tile, with at least the x, y, wd, ht,
	// and coords[4] fields set. The latter are the *world-projected*
	// coordinates of the tile corners.

	// Not even visible? Give up:
	if (!tile_is_visible(tile))
		return;

	// Tile is already the smallest possible size, cannot split, must accept:
	if (tile->wd <= minsize) {
		union vec vzooms = zooms_quad(
			world_zoom,
			vertex_all_x(tile->vertex),
			vertex_all_y(tile->vertex),
			vertex_all_z(tile->vertex)
		);
		tile->zoom = vec_imax(vzooms);
		drawlist_add(tile);
		return;
	}
	float halfsize = ldexpf(tile->wd, -1);

	// Split tile up in four quadrants:
	//
	//   0---1---2
	//   | 0 | 1 |
	//   7---8---3
	//   | 3 | 2 |
	//   6---5---4
	//
	// We inherit vertices 0, 2, 4 and 6 from the parent tile.
	// The other vertices need to be generated and projected:
	struct vertex vertex_new[5] = {
		{ .tile = { tile->x + halfsize, tile->y,           } },	// 1
		{ .tile = { tile->x + tile->wd, tile->y + halfsize } },	// 3
		{ .tile = { tile->x + halfsize, tile->y + tile->ht } },	// 5
		{ .tile = { tile->x,            tile->y + halfsize } },	// 7
		{ .tile = { tile->x + halfsize, tile->y + halfsize } },	// 8
	};

	// Generate a list of vertices combining the parent tile with
	// the new vertices, numbered according to the diagram above:
	struct vertex *vertex[9] = {
		&tile->vertex[0],
		&vertex_new[0],
		&tile->vertex[1],
		&vertex_new[1],
		&tile->vertex[2],
		&vertex_new[2],
		&tile->vertex[3],
		&vertex_new[3],
		&vertex_new[4],
	};

	// For the four quads, define corner vertices:
	struct quad quad[4] = {
		{ false, { vertex[0], vertex[1], vertex[8], vertex[7] } },
		{ false, { vertex[1], vertex[2], vertex[3], vertex[8] } },
		{ false, { vertex[8], vertex[3], vertex[4], vertex[5] } },
		{ false, { vertex[7], vertex[8], vertex[5], vertex[6] } },
	};

	// Also define nine rectangles made up out of various quads:
	//
	//   0-------2   0-------2   0---1---2   0---1---2
	//   |       |   |   1   |   |   |   |   | 5 | 6 |
	//   |   0   |   7-------3   | 3 | 4 |   7---8---3
	//   |       |   |   2   |   |   |   |   | 8 | 7 |
	//   6-------4   6-------4   6---5---4   6---5---4
	//
	struct rect rect[9] = {
		{ tile->wd, tile->ht, { &quad[0], &quad[1], &quad[2], &quad[3] }, { vertex[0], vertex[2], vertex[4], vertex[6] } },
		{ tile->wd, halfsize, { &quad[0], &quad[1], NULL, NULL }, { vertex[0], vertex[2], vertex[3], vertex[7] } },
		{ tile->wd, halfsize, { &quad[3], &quad[2], NULL, NULL }, { vertex[7], vertex[3], vertex[4], vertex[6] } },
		{ halfsize, tile->ht, { &quad[0], &quad[3], NULL, NULL }, { vertex[0], vertex[1], vertex[5], vertex[6] } },
		{ halfsize, tile->ht, { &quad[1], &quad[2], NULL, NULL }, { vertex[1], vertex[2], vertex[4], vertex[5] } },
		{ halfsize, halfsize, { &quad[0], NULL, NULL, NULL }, { vertex[0], vertex[1], vertex[8], vertex[7] } },
		{ halfsize, halfsize, { &quad[1], NULL, NULL, NULL }, { vertex[1], vertex[2], vertex[3], vertex[8] } },
		{ halfsize, halfsize, { &quad[2], NULL, NULL, NULL }, { vertex[8], vertex[3], vertex[4], vertex[5] } },
		{ halfsize, halfsize, { &quad[3], NULL, NULL, NULL }, { vertex[7], vertex[8], vertex[5], vertex[6] } },
	};

	// Project the new vertices:
	for (int i = 0; i < 5; i++)
		project(&vertex_new[i]);

	// Get zooms for new vertices:
	union vec zooms_new = zooms_quad(
		world_zoom,
		vertex_all_x(vertex_new),
		vertex_all_y(vertex_new),
		vertex_all_z(vertex_new)
	);

	// Assign zooms:
	for (int i = 0; i < 4; i++)
		vertex_new[i].zoom = zooms_new.elem.i[i];

	// Don't forget the zoom for the midpoint:
	vertex_new[4].zoom = zoom_point(world_zoom, &vertex_new[4].coords);

	const struct coords *center = world_get_center();

	// Loop over all rects:
	for (int i = 0; i < 9; i++)
	{
		float rect_x = rect[i].vertex[0]->tile.x;
		float rect_y = rect[i].vertex[0]->tile.y;

		// Sign test of the four corner points: all x's or all y's should lie
		// to one side of the center, else do not accept:
		bool signs_x_equal = ((rect_x < center->tile.x) == ((rect_x + rect[i].wd) < center->tile.x));
		bool signs_y_equal = ((rect_y < center->tile.y) == ((rect_y + rect[i].ht) < center->tile.y));

		if (!signs_x_equal && !signs_y_equal)
			continue;

		if (!rect_quads_unused(&rect[i]))
			continue;

		// If not all vertices in the rect have the same zoomlevel,
		// the rect is not "homogeneous" and we need to split it:
		if (!rect_has_single_zoomlevel(&rect[i]))
			continue;

		struct tile t = {
			.x = rect_x,
			.y = rect_y,
			.wd = rect[i].wd,
			.ht = rect[i].ht,
			.zoom = rect[i].vertex[0]->zoom,
		};

		for (int j = 0; j < 4; j++)
			memcpy(&t.vertex[j], rect[i].vertex[j], sizeof(struct vertex));

		if (!tile_is_visible(&t))
			continue;

		if (!tile_edges_agree(&t))
			continue;

		drawlist_add(&t);
		rect_quads_mark_used(&rect[i]);
	}

	// Double the minsize:
	if (minsize < 1.0f)
		minsize = ldexpf(minsize, 1);

	// Split all remaining quads recursively:
	for (int i = 0; i < 4; i++)
	{
		if (quad[i].used)
			continue;

		struct tile t = {
			.x = quad[i].vertex[0]->tile.x,
			.y = quad[i].vertex[0]->tile.y,
			.wd = halfsize,
			.ht = halfsize,
			.zoom = quad[i].vertex[0]->zoom,
		};

		for (int j = 0; j < 4; j++)
			memcpy(&t.vertex[j], quad[i].vertex[j], sizeof(struct vertex));

		// If quad is visible, recursively divide it:
		if (tile_is_visible(&t))
			reduce_block(&t, maxzoom + 1, minsize);
	}
}

static void
optimize_block (int x, int y, int relzoom)
{
	int sz = (1 << relzoom);
	int z = world_zoom - relzoom;
	int have_smaller = 0;
	struct tile *tile1, *tile2;

	#define inherit_vertices(a,b,u,v)							\
		memcpy(&tile##a->vertex[u], &tile##b->vertex[u], sizeof(struct vertex));	\
		memcpy(&tile##a->vertex[v], &tile##b->vertex[v], sizeof(struct vertex))

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
					inherit_vertices(1,2,2,3);
					drawlist_detach(tile2);
					goto reset;
				}
				if (tile2->y + tile2->ht == tile1->y) {
					tile2->ht += tile1->ht;
					inherit_vertices(2,1,2,3);
					drawlist_detach(tile1);
					goto reset;
				}
			}
			// Do tiles share height and are they horizontal neighbours?
			if (tile1->ht == tile2->ht && tile1->y == tile2->y) {
				if (tile1->x + tile1->wd == tile2->x) {
					tile1->wd += tile2->wd;
					inherit_vertices(1,2,1,2);
					drawlist_detach(tile2);
					goto reset;
				}
				if (tile2->x + tile2->wd == tile1->x) {
					tile2->wd += tile1->wd;
					inherit_vertices(2,1,1,2);
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

bool
tilepicker_next (struct tilepicker *tile)
{
	// Returns true or false depending on whether a tile is available and
	// returned in the pointer arguments.
	if (drawlist_iter == NULL)
		return false;

	for (int i = 0; i < 4; i++) {
		memcpy(&tile->coords[i], &drawlist_iter->vertex[i].coords, sizeof(union vec));
		memcpy(&tile->normal[i], &drawlist_iter->vertex[i].normal, sizeof(union vec));
	}

	tile->pos.x	= drawlist_iter->x;
	tile->pos.y	= drawlist_iter->y;
	tile->size.wd	= drawlist_iter->wd;
	tile->size.ht	= drawlist_iter->ht;
	tile->zoom	= drawlist_iter->zoom;

	drawlist_iter = drawlist_iter->next;
	return true;
}

bool
tilepicker_first (struct tilepicker *tile)
{
	// Reset iterator:
	drawlist_iter = drawlist;

	return tilepicker_next(tile);
}

void
tilepicker_destroy (void)
{
	mempool_destroy();
}
