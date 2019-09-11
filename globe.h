#pragma once

#include <stdbool.h>

#include <vec/vec.h>

#include "cache.h"

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
extern void globe_tile_to_sphere (const struct cache_node *n, struct cache_data *d);
extern bool globe_intersect (const union vec *start, const union vec *dir, float *lat, float *lon);
extern void globe_moveto (const double lat, const double lon);
extern void globe_init (void);
