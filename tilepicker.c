#include <stdbool.h>
#include "world.h"
#include "viewport.h"

static int world_zoom;
static int tile_top;
static int tile_left;
static int tile_right;
static int tile_bottom;
static int tile_last;
static double center_x;
static double center_y;

static int iter_x;
static int iter_y;

static int find_farthest_tile (void);
static int tile_get_zoom (int tile_x, int tile_y);

void
tilepicker_recalc (void)
{
	// This function first calculates values and puts them in global
	// variables, so that the other routines in this module can use them.
	// I'm normally not a fan of globals, but this does make the code
	// simpler.
	world_zoom = world_get_zoom();

	tile_top = viewport_get_tile_top();
	tile_left = viewport_get_tile_left();
	tile_right = viewport_get_tile_right();
	tile_bottom = viewport_get_tile_bottom();
	tile_last = world_get_size() - 1;

	center_x = viewport_get_center_x();
	center_y = viewport_get_center_y();

	int fx, fy;
	switch (find_farthest_tile()) {
		case 0: fx = tile_left; fy = tile_top; break;
		case 1: fx = tile_right; fy = tile_top; break;
		case 2: fx = tile_right; fy = tile_bottom; break;
		case 3: fx = tile_left; fy = tile_bottom; break;
	}
	int lowzoom = tile_get_zoom(fx, fy);
}

static bool
tile_is_visible (int x, int y, int wd, int ht)
{
	// Check if at least one point of the tile is inside the frustum.
	// This is expensive, but not as expensive as procuring and rendering
	// an unnecessary tile.
	if (point_inside_frustum(x,      tile_last - y)
	 || point_inside_frustum(x + wd, tile_last - y)
	 || point_inside_frustum(x,      tile_last - y + ht)
	 || point_inside_frustum(x + wd, tile_last - y + ht)) {
		return true;
	}
	return false;
}

static int
find_farthest_tile (void)
{
	// Find the tile farthest away from the viewport.
	// This is the smallest tile rendered, and hence must be a tile at the
	// lowest zoom level. It must be one of the corner tiles of the
	// bounding box. We cheat slightly by using the distance from the
	// center point, not the actual pixel dimensions of the tile.

	float dx, dy;
	float dist[4];
	float max = 0.0;
	int num;

	// Left top:
	dx = (float)center_x - (float)tile_left + 0.5;
	dy = (float)center_y - (float)tile_top + 0.5;
	dist[0] = dx * dx + dy * dy;

	// Right top:
	dx = (float)center_x - (float)tile_right + 0.5;
	dy = (float)center_y - (float)tile_top + 0.5;
	dist[1] = dx * dx + dy * dy;

	// Right bottom:
	dx = (float)center_x - (float)tile_right + 0.5;
	dy = (float)center_y - (float)tile_bottom + 0.5;
	dist[2] = dx * dx + dy * dy;

	// Left bottom:
	dx = (float)center_x - (float)tile_left + 0.5;
	dy = (float)center_y - (float)tile_bottom + 0.5;
	dist[3] = dx * dx + dy * dy;

	for (int i = 0; i < 4; i++) {
		if (dist[i] > max) {
			max = dist[i];
			num = i;
		}
	}
	// 0 is top left
	// 1 is top right
	// 2 is bottom right
	// 3 is bottom left
	// (clockwise from top left)
	return num;
}

static int
tile_get_zoom (int tile_x, int tile_y)
{
	// Shortcut: if the tilt is exactly 0.0, always use world zoom:
	if (viewport_get_tilt() == 0.0) {
		return world_zoom;
	}
	float tilex = tile_x - (float)center_x + 0.5;
	float tiley = tile_last - tile_y - (float)center_y + 0.5;

	// Use *cubed* distance: further tiles are proportionally smaller:
	float dist = tilex * tilex + tiley * tiley;

	// The 40.0 is a fudge factor for the size of the "zoom horizon":
	int zoom = (float)(world_zoom + 1) - dist / 40.0;

	return (zoom < 0) ? 0 : zoom;
}

bool
tilepicker_next (int *x, int *y, int *wd, int *ht, int *zoom)
{
	// Returns true or false depending on whether a tile is available and
	// returned in the pointer arguments.
	do {
		if (iter_y-- == tile_top) {
			iter_y = tile_bottom;
			if (iter_x++ == tile_right) {
				return false;
			}
		}
		*x = iter_x;
		*y = iter_y;
		*wd = 1;
		*ht = 1;
		*zoom = tile_get_zoom(iter_x, iter_y);
	}
	while (!tile_is_visible(iter_x, iter_y, 1, 1));

	return true;
}

bool
tilepicker_first (int *x, int *y, int *wd, int *ht, int *zoom)
{
	iter_x = *x = tile_left;
	iter_y = *y = tile_bottom;
	*wd = 1;
	*ht = 1;
	*zoom = tile_get_zoom(iter_x, iter_y);

	// Ensure first tile is also a visible one:
	while (!tile_is_visible(iter_x, iter_y, 1, 1)) {
		if (!tilepicker_next(x, y, wd, ht, zoom)) {
			return false;
		}
	}
	return true;
}
