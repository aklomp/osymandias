#pragma once

#include <stdbool.h>

#include <vec/vec.h>

#include "cache.h"

// A point on the unit sphere (the globe) in 3D space. Packed and aligned so it
// can be directly uploaded to the GPU.
struct globe_point {
	float x;
	float y;
	float z;
} __attribute__((packed,aligned(4)));

// 3D spherical coordinates of a tile's four vertices on the unit sphere. The
// vertices start bottom left (southwest) and run counterclockwise.
struct globe_tile {
	struct globe_point sw;
	struct globe_point se;
	struct globe_point ne;
	struct globe_point nw;
} __attribute__((packed,aligned(4)));

// The globe model's public state structure.
struct globe {

	// Cursor position in radians:
	double lat;
	double lon;

	// Model matrix and components:
	struct {
		double model[16];
		struct {
			double lat[16];
			double lon[16];
		} rotate;
	} matrix;

	// Inverse matrices:
	struct {
		double model[16];
		struct {
			double lat[16];
			double lon[16];
		} rotate;
	} invert;

	// Matrix update flags:
	struct {
		bool model;
	} updated;
};

extern const struct globe *globe_get (void);
extern void globe_updated_reset (void);
extern void globe_map_tile (const struct cache_node *n, struct globe_tile *t);
extern bool globe_intersect (const union vec *start, const union vec *dir, float *lat, float *lon);
extern void globe_moveto (const double lat, const double lon);
extern void globe_init (void);
