#pragma once

#include <vec/vec.h>

#include "camera.h"
#include "vec.h"
#include "worlds.h"

// The zoom decay determines the rate by which the zoom level drops off with distance:
#define ZOOM_DECAY	50.0

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
