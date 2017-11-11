#include <vec/vec.h>

#include "vec.h"

// The zoom decay determines the rate by which the zoom level drops off with distance:
#define ZOOM_DECAY	50.0

static inline union vec
tile2d_get_corner_zooms_abs (int x1, int y1, int x2, int y2, const struct coords *center, int world_zoom)
{
	// The corner tiles are the 1x1 unit tiles at the edges of this larger
	// square tile; the distance from the center point determines the zoom level
	// at which they should be shown:
	//
	// x1 -> x2
	// +-----+ y1
	// |'   '| |     (center_x, center_y)
	// |,   ,| V              .
	// +-----+ y2
	//
	union vec x = vec_to_float(vec_i(x1, x2 - 1, x2 - 1, x1));
	union vec y = vec_to_float(vec_i(y1, y1, y2 - 1, y2 - 1));

	// Translate tile coordinates to world coordinates:
	x = vec_add(x, vec_1(0.5f - center->tile.x));
	y = vec_add(y, vec_1(0.5f - center->tile.y));

	// Use distance-squared, save a square root:
	union vec d = camera_distance_squared_quad(x, y, vec_zero());

	// Scale the distances down to zoom levels and convert to int:
	union vec vz = vec_to_int(vec_div(d, vec_1(ZOOM_DECAY)));

	// Turn distance gradients into absolute zoom levels:
	vz = vec_isub(vec_i1(world_zoom), vz);

	// Clip negative absolute zooms to zero:
	return vec_and(vz, vec_igt(vz, vec_zero()));
}

static inline union vec
tile2d_get_corner_zooms (int x, int y, int sz, const struct coords *center, int world_zoom)
{
	return tile2d_get_corner_zooms_abs(x, y, x + sz, y + sz, center, world_zoom);
}

static inline int
tile2d_get_zoom (int x, int y, int world_zoom)
{
	// Use centerpoint of tile as reference:
	union vec point = vec(x + 0.5f, world_get_size() - (y + 0.5f), 0.0f, 0.0f);

	// Calculate squared distance from camera position, scale to zoomlevel:
	int dist = camera_distance_squared(&point) / ZOOM_DECAY;

	// Find absolute zoomlevel by subtracting it from the current world zoom:
	int zoom = world_zoom - dist;

	// Clip to 0 if negative:
	return (zoom < 0) ? 0 : zoom;
}

static inline int
tile2d_get_max_zoom (int x, int y, int sz, const struct coords *center, int world_zoom)
{
	// For the given quad, find the tile nearest to center:
	int rx = (center->tile.x > x + sz) ? x + sz : (center->tile.x < x) ? x : center->tile.x + 0.5;
	int ry = (center->tile.y > y + sz) ? y + sz : (center->tile.y < y) ? y : center->tile.y + 0.5;

	return tile2d_get_zoom(rx, ry, world_zoom);
}

static inline union vec
zooms_quad (int world_zoom, const union vec x, const union vec y, const union vec z)
{
	// Get distance squared from camera location:
	union vec d = camera_distance_squared_quad(x, y, z);

	// Scale down to zoom gradients, convert to int:
	union vec vz = vec_to_int(vec_div(d, vec_1(ZOOM_DECAY)));

	// Turn distance gradient into absolute zoom levels:
	vz = vec_isub(vec_i1(world_zoom), vz);

	// Clip negative zooms to zero:
	return vec_and(vz, vec_igt(vz, vec_zero()));
}

static inline int
zoom_edges_highest (int world_zoom, const union vec x, const union vec y, const union vec z)
{
	// Get squared distances from camera to each edge line:
	union vec d = camera_distance_squared_quadedge(x, y, z);

	// Scale down to zoom gradients, convert to int:
	union vec vz = vec_to_int(vec_div(d, vec_1(ZOOM_DECAY)));

	// Turn into absolute zoom levels:
	vz = vec_isub(vec_i1(world_zoom), vz);

	// Clip negative zooms to zero:
	vz = vec_and(vz, vec_igt(vz, vec_zero()));

	return vec_imax(vz);
}

static inline int
zoom_point (int world_zoom, const union vec *v)
{
	float f = camera_distance_squared(v);
	int r = world_zoom - (f / ZOOM_DECAY);

	return (r < 0) ? 0 : r;
}
