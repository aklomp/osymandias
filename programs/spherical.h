#pragma once

#include "../cache.h"

struct program_spherical {
	const float *cam;
	const float *mat_viewproj;
	const float *mat_model;
	const float *mat_model_inv;
	const float *mat_view_inv;
	float vp_angle;
	float vp_width;
};

extern void program_spherical_set_tile (const struct cache_node *tile, const struct cache_data *data);
extern void program_spherical_use (const struct program_spherical *);
