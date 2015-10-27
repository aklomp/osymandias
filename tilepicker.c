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
#include "coord3d.h"

#define MEMPOOL_BLOCK_SIZE 100

// The minimal number of subdivisions the globe sphere should have at the
// lowest zoom levels, as a power of two. At zoom level 0, the world is a
// single tile in size. In order to keep the spherical appearance at that
// level, and to make the visibility logic work, we split the sphere into
// (2 << MIN_SUBDIVISIONS) subdivisions.
#define MIN_SUBDIVISIONS 4

struct tile {
	float x;
	float y;
	float wd;
	float ht;
	int zoom;
	struct vector p[4];
	struct vector normal[4];
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
static double center_x_dbl;
static double center_y_dbl;

static struct tile *drawlist = NULL;
static struct tile *drawlist_tail = NULL;
static struct tile *drawlist_iter = NULL;

static struct mempool_block *mempool = NULL;
static struct mempool_block *mempool_tail = NULL;
static int mempool_idx = 0;

static void reduce_block (struct tile *tile, int maxzoom, float minsize);
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

static inline void
project_points_planar (struct vector points[], struct vector normals[], int npoints)
{
	// Each "point" comes as three floats: x, y and z:
	for (int i = 0; i < npoints; i++) {
		points[i].x -= center_x_dbl;
		points[i].y -= center_y_dbl;
		points[i].y = -points[i].y;

		normals[i].x = 0.0f;
		normals[i].y = 0.0f;
		normals[i].z = 1.0f;
		normals[i].w = 0.0f;
	}
}

static inline void
project_points_spherical (struct vector points[], struct vector normals[], int npoints)
{
	double cx_lon, sin_cy_lat, cos_cy_lat;

	// Precalculate some invariants outside the loop:
	tilepoint_to_xyz_precalc(world_size, center_x_dbl, center_y_dbl, &cx_lon, &sin_cy_lat, &cos_cy_lat);

	// Each "point" comes as three floats: x, y and z:
	for (int i = 0; i < npoints; i++)
		tilepoint_to_xyz(points[i].x, points[i].y, world_size, cx_lon, sin_cy_lat, cos_cy_lat, &points[i], &normals[i]);
}

static bool
tile_is_visible (struct tile *const tile)
{
	return camera_visible_quad(&tile->p[0]);
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

static bool
tile_edges_agree (struct tile *const tile)
{
	return (tile->zoom == zoom_edges_highest(
		world_zoom,
		(vec4f){ tile->p[0].x, tile->p[1].x, tile->p[2].x, tile->p[3].x },
		(vec4f){ tile->p[0].y, tile->p[1].y, tile->p[2].y, tile->p[3].y },
		(vec4f){ tile->p[0].z, tile->p[1].z, tile->p[2].z, tile->p[3].z }));
}

static void
tilepicker_planar (void)
{
	int lowzoom;

	// In the planar world, we have the concepts of "leftmost tile",
	// "rightmost tile", and so on. Also the concept of the "farthest
	// visible tile", which by definition must have the lowest zoomlevel
	// of all visible tiles. This knowledge lets us take some shortcuts.

	tile_top = viewport_get_tile_top();
	tile_left = viewport_get_tile_left();
	tile_right = viewport_get_tile_right();
	tile_bottom = viewport_get_tile_bottom();

	tile_farthest_get_zoom(&lowzoom);

	// Start with biggest block size in world; recursively cut these into smaller blocks:
	int zoomdiff = world_zoom - lowzoom;
	int tilesize = 1 << zoomdiff;
	for (int tile_x = tile_left & ~(tilesize - 1); tile_x <= tile_right; tile_x += tilesize) {
		for (int tile_y = tile_top & ~(tilesize - 1); tile_y <= tile_bottom; tile_y += tilesize) {
			struct tile tile = {
				.x = tile_x,
				.y = tile_y,
				.wd = tilesize,
				.ht = tilesize,
				.p = {
					{ tile_x,            tile_y,            0.0f, 1.0f },
					{ tile_x + tilesize, tile_y,            0.0f, 1.0f },
					{ tile_x + tilesize, tile_y + tilesize, 0.0f, 1.0f },
					{ tile_x,            tile_y + tilesize, 0.0f, 1.0f }
				},
			};
			// Must call reduce_block with a tile structure containing
			// the four projected corner points, so bootstrap:
			project_points_planar(tile.p, tile.normal, 4);

			// Recursively split this block:
			reduce_block(&tile, lowzoom, 1.0f);

			// Merge adjacent blocks with same zoom level:
			optimize_block(tile_x, tile_y, zoomdiff);
		}
	}
}

static void
tilepicker_spherical (void)
{
	// In the spherical world, we start off at the lowest zoom level,
	// level 0, where the world is a single tile. We split the world
	// into (1 << MIN_SUBDIVISIONS) subdivisions to keep the spherical
	// character of the globe. We recursively walk over the visible
	// tiles and add them to the drawlist:

	int minsize = (world_zoom > MIN_SUBDIVISIONS) ? 0 : (world_zoom - MIN_SUBDIVISIONS);

	for (int tile_y = 0; tile_y < (1 << MIN_SUBDIVISIONS); tile_y++) {
		for (int tile_x = 0; tile_x < (1 << MIN_SUBDIVISIONS); tile_x++) {
			// TODO: hoist invariants out of loop
			float x = ldexpf(tile_x, world_zoom - MIN_SUBDIVISIONS);
			float y = ldexpf(tile_y, world_zoom - MIN_SUBDIVISIONS);
			float sz = ldexpf(1.0f, world_zoom - MIN_SUBDIVISIONS);

			struct tile tile = {
				.x = x,
				.y = y,
				.wd = sz,
				.ht = sz,
				.p = {
					{ x,      y,      0.0f, 1.0f },
					{ x + sz, y,      0.0f, 1.0f },
					{ x + sz, y + sz, 0.0f, 1.0f },
					{ x,      y + sz, 0.0f, 1.0f }
				}
			};
			project_points_spherical(tile.p, tile.normal, 4);

			reduce_block(&tile, 0, ldexpf(1.0f, minsize));
		}
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

	// Convert center_y to *tile* coordinates; much easier to work with:
	center_x = center_x_dbl = viewport_get_center_x();
	center_y = center_y_dbl = world_size - viewport_get_center_y();

	// Reset drawlist pointers. The drawlist consists of tiles allocated in
	// the mempool that point to each other; the drawlist itself does not
	// involve allocations.
	drawlist = drawlist_tail = NULL;

	// Reset tile mempool pointers. For this round, start giving out tiles
	// from the top of the mempool again.
	mempool_reset();

	(viewport_mode_get() == VIEWPORT_MODE_PLANAR)
		? tilepicker_planar()
		: tilepicker_spherical();
}

static void
reduce_block (struct tile *tile, int maxzoom, float minsize)
{
	#define inherit_point(a,k)			\
		memcpy(&t.p[a],				\
			  (k == 0) ? &tile->p[0]	\
			: (k == 1) ? &p[0]		\
			: (k == 2) ? &tile->p[1]	\
			: (k == 3) ? &p[1]		\
			: (k == 4) ? &tile->p[2]	\
			: (k == 5) ? &p[2]		\
			: (k == 6) ? &tile->p[3]	\
			: (k == 7) ? &p[3]		\
			: &p[4]				\
			, sizeof(struct vector));	\
		memcpy(&t.normal[a],			\
			  (k == 0) ? &tile->normal[0]	\
			: (k == 1) ? &normal[0]		\
			: (k == 2) ? &tile->normal[1]	\
			: (k == 3) ? &normal[1]		\
			: (k == 4) ? &tile->normal[2]	\
			: (k == 5) ? &normal[2]		\
			: (k == 6) ? &tile->normal[3]	\
			: (k == 7) ? &normal[3]		\
			: &normal[4]			\
			, sizeof(struct vector))

	#define setcorners(a,b,c,d) \
		inherit_point(0,a); \
		inherit_point(1,b); \
		inherit_point(2,c); \
		inherit_point(3,d);

	// Caller passes us a struct tile, with at least the x, y, wd, ht,
	// and p[4][3] fields set. The latter are the *world-projected*
	// coordinates of the tile corners.

	// Not even visible? Give up:
	if (!tile_is_visible(tile)) {
		return;
	}
	// Tile is already the smallest possible size, cannot split, must accept:
	if (tile->wd <= minsize) {
		vec4i vzooms = zooms_quad(
			world_zoom,
			(vec4f){ tile->p[0].x, tile->p[1].x, tile->p[2].x, tile->p[3].x },
			(vec4f){ tile->p[0].y, tile->p[1].y, tile->p[2].y, tile->p[3].y },
			(vec4f){ tile->p[0].z, tile->p[1].z, tile->p[2].z, tile->p[3].z }
		);
		tile->zoom = vec4i_hmax(vzooms);
		drawlist_add(tile);
		return;
	}
	float halfsize = ldexpf(tile->wd, -1);

	// Split tile up in four quadrants:
	//
	//  0--1--2
	//  | 0| 1|
	//  7--8--3
	//  | 3| 2|
	//  6--5--4
	//
	// Need to project points 1, 3, 5, 7, and 8, and find their zoom levels:
	struct vector p[5] = {
		{ tile->x + halfsize, tile->y,            0.0f, 1.0f },	// 1
		{ tile->x + tile->wd, tile->y + halfsize, 0.0f, 1.0f },	// 3
		{ tile->x + halfsize, tile->y + tile->ht, 0.0f, 1.0f },	// 5
		{ tile->x,            tile->y + halfsize, 0.0f, 1.0f },	// 7
		{ tile->x + halfsize, tile->y + halfsize, 0.0f, 1.0f }	// 8
	};

	// Also save the normals:
	struct vector normal[5];

	(viewport_mode_get() == VIEWPORT_MODE_PLANAR)
		? project_points_planar(p, normal, 5)
		: project_points_spherical(p, normal, 5);

	// And the midpoint zoom, shared by all quadrants in this tile:
	int mzoom = zoom_point(world_zoom, &p[4]);

	// Sign test of the four corner points: all x's or all y's should lie
	// to one side of the center, else split up in four quadrants of the same zoom level:
	bool signs_x_equal = ((tile->x < center_x) == ((tile->x + tile->wd) < center_x));
	bool signs_y_equal = ((tile->y < center_y) == ((tile->y + tile->ht) < center_y));

	// If the mzoom is larger than the max allowed zoom, we must recurse to the next zoom level.
	// If the tile is under the cursor, we must also split it:
	if (mzoom > maxzoom || (!(signs_x_equal || signs_y_equal))) {
		// Double the minsize:
		if (minsize < 1.0f) {
			minsize = ldexpf(minsize, 1);
		}
		{	struct tile t = { .x = tile->x, .y = tile->y, .wd = halfsize, .ht = halfsize };
			setcorners(0, 1, 8, 7);
			reduce_block(&t, maxzoom + 1, minsize);
		}
		{	struct tile t = { .x = tile->x + halfsize, .y = tile->y, .wd = halfsize, .ht = halfsize };
			setcorners(1, 2, 3, 8);
			reduce_block(&t, maxzoom + 1, minsize);
		}
		{	struct tile t = { .x = tile->x + halfsize, .y = tile->y + halfsize, .wd = halfsize, .ht = halfsize };
			setcorners(8, 3, 4, 5);
			reduce_block(&t, maxzoom + 1, minsize);
		}
		{	struct tile t = { .x = tile->x, .y = tile->y + halfsize, .wd = halfsize, .ht = halfsize };
			setcorners(7, 8, 5, 6);
			reduce_block(&t, maxzoom + 1, minsize);
		}
		return;
	}
	// Get vertex zoomlevels:
	vec4i vzooms = zooms_quad(
		world_zoom,
		(vec4f){ tile->p[0].x, tile->p[1].x, tile->p[2].x, tile->p[3].x },
		(vec4f){ tile->p[0].y, tile->p[1].y, tile->p[2].y, tile->p[3].y },
		(vec4f){ tile->p[0].z, tile->p[1].z, tile->p[2].z, tile->p[3].z }
	);
	// If the whole tile has the same zoom level, add:
	if (vec4i_all_same(vzooms) && vzooms[0] == mzoom
	&& zoom_edges_highest(
		world_zoom,
		(vec4f){ tile->p[0].x, tile->p[1].x, tile->p[2].x, tile->p[3].x },
		(vec4f){ tile->p[0].y, tile->p[1].y, tile->p[2].y, tile->p[3].y },
		(vec4f){ tile->p[0].z, tile->p[1].z, tile->p[2].z, tile->p[3].z }
	) == mzoom) {
		tile->zoom = mzoom;
		drawlist_add(tile);
		return;
	}
	// 4/9 of the points are in tile.p, the rest is in p.
	// Let's get zooms for the points in p:
	vec4i pzooms = zooms_quad(
		world_zoom,
		(vec4f){ p[0].x, p[1].x, p[2].x, p[3].x },
		(vec4f){ p[0].y, p[1].y, p[2].y, p[3].y },
		(vec4f){ p[0].z, p[1].z, p[2].z, p[3].z }
	);
	// Combine vzooms and pzooms into one vec8i:
	vec8i zooms = vec8i_from_vec4i(vzooms, pzooms);

	// zooms now contains the zoomlevels of the eight points in a
	// strange order: { 0, 2, 4, 6, 1, 3, 5, 7 }

	// Define quad masks. Each quad uses point 8, but also these:
	// point #      0     2     4     6     1     3     5     7
	vec8i q0 = { 0xff, 0x00, 0x00, 0x00, 0xff, 0x00, 0x00, 0xff };
	vec8i q1 = { 0x00, 0xff, 0x00, 0x00, 0xff, 0xff, 0x00, 0x00 };
	vec8i q2 = { 0x00, 0x00, 0xff, 0x00, 0x00, 0xff, 0xff, 0x00 };
	vec8i q3 = { 0x00, 0x00, 0x00, 0xff, 0x00, 0x00, 0xff, 0xff };

	// Subtract the zoom for point 8 from all these points:
	zooms -= vec8i_int(mzoom);

	// Now we start having fun. If zooms is now all zero, the whole
	// quad has the same zoomlevel:
	int free[4] = { 1, 1, 1, 1 };

	// OK, split tile up in four quadrants:
	// Check the top two quadrants, q0 and q1, for same zoom:
	if (vec8i_all_zero((q0 | q1) & zooms)) {
		struct tile t = { .x = tile->x, .y = tile->y, .wd = tile->wd, .ht = halfsize, .zoom = mzoom };
		setcorners(0, 2, 3, 7);
		if (tile_is_visible(&t) && tile_edges_agree(&t)) {
			drawlist_add(&t);
			free[0] = free[1] = 0;
		}
	}
	// Check bottom two quadrants, q2 and q3, for same zoom:
	if (vec8i_all_zero((q2 | q3) & zooms)) {
		struct tile t = { .x = tile->x, .y = tile->y + halfsize, .wd = tile->wd, .ht = halfsize, .zoom = mzoom };
		setcorners(7, 3, 4, 6);
		if (tile_is_visible(&t) && tile_edges_agree(&t)) {
			drawlist_add(&t);
			free[2] = free[3] = 0;
		}
	}
	// Check left two quadrants, q0 and q3, for same zoom:
	if (free[0] && free[3] && vec8i_all_zero((q0 | q3) & zooms)) {
		struct tile t = { .x = tile->x, .y = tile->y, .wd = halfsize, .ht = tile->ht, .zoom = mzoom };
		setcorners(0, 1, 5, 6);
		if (tile_is_visible(&t) && tile_edges_agree(&t)) {
			drawlist_add(&t);
			free[0] = free[3] = 0;
		}
	}
	// Check right two quadrants, q1 and q2, for same zoom:
	if (free[1] && free[2] && vec8i_all_zero((q1 | q2) & zooms)) {
		struct tile t = { .x = tile->x + halfsize, .y = tile->y, .wd = halfsize, .ht = tile->ht, .zoom = mzoom };
		setcorners(1, 2, 4, 5);
		if (tile_is_visible(&t) && tile_edges_agree(&t)) {
			drawlist_add(&t);
			free[1] = free[2] = 0;
		}
	}
	// Double the minsize:
	if (minsize < 1.0f) {
		minsize = ldexpf(minsize, 1);
	}
	// Check the individual, still-free quadrants:
	if (free[0]) {
		struct tile t = { .x = tile->x, .y = tile->y, .wd = halfsize, .ht = halfsize, .zoom = mzoom };
		setcorners(0, 1, 8, 7);
		if (vec8i_all_zero(q0 & zooms) && tile_is_visible(&t) && tile_edges_agree(&t)) {
			drawlist_add(&t);
		}
		else reduce_block(&t, maxzoom + 1, minsize);
	}
	if (free[1]) {
		struct tile t = { .x = tile->x + halfsize, .y = tile->y, .wd = halfsize, .ht = halfsize, .zoom = mzoom };
		setcorners(1, 2, 3, 8);
		if (vec8i_all_zero(q1 & zooms) && tile_is_visible(&t) && tile_edges_agree(&t)) {
			drawlist_add(&t);
		}
		else reduce_block(&t, maxzoom + 1, minsize);
	}
	if (free[2]) {
		struct tile t = { .x = tile->x + halfsize, .y = tile->y + halfsize, .wd = halfsize, .ht = halfsize, .zoom = mzoom };
		setcorners(8, 3, 4, 5);
		if (vec8i_all_zero(q2 & zooms) && tile_is_visible(&t) && tile_edges_agree(&t)) {
			drawlist_add(&t);
		}
		else reduce_block(&t, maxzoom + 1, minsize);
	}
	if (free[3]) {
		struct tile t = { .x = tile->x, .y = tile->y + halfsize, .wd = halfsize, .ht = halfsize, .zoom = mzoom };
		setcorners(7, 8, 5, 6);
		if (vec8i_all_zero(q3 & zooms) && tile_is_visible(&t) && tile_edges_agree(&t)) {
			drawlist_add(&t);
		}
		else reduce_block(&t, maxzoom + 1, minsize);
	}
}

static void
optimize_block (int x, int y, int relzoom)
{
	int sz = (1 << relzoom);
	int z = world_zoom - relzoom;
	int have_smaller = 0;
	struct tile *tile1, *tile2;

	#define inherit_vertices(a,b,u,v)					\
		memcpy(&tile##a->p[u], &tile##b->p[u], sizeof(struct vector));	\
		memcpy(&tile##a->p[v], &tile##b->p[v], sizeof(struct vector))

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
tilepicker_next (float *x, float *y, float *wd, float *ht, int *zoom, struct vector coords[4], struct vector normal[4])
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

	memcpy(coords, &drawlist_iter->p, sizeof(struct vector[4]));
	memcpy(normal, &drawlist_iter->normal, sizeof(struct vector[4]));

	drawlist_iter = drawlist_iter->next;
	return true;
}

bool
tilepicker_first (float *x, float *y, float *wd, float *ht, int *zoom, struct vector coords[4], struct vector normal[4])
{
	// Reset iterator:
	drawlist_iter = drawlist;

	return tilepicker_next(x, y, wd, ht, zoom, coords, normal);
}

void
tilepicker_destroy (void)
{
	mempool_destroy();
}
