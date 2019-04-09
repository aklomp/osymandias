#pragma once

#include <stdint.h>

struct program_frustum {
	const float *mat_proj;
	const float *mat_viewproj;
	const float *mat_model;
	const float *mat_view_inv;
	const float *mat_model_inv;
	int world_size;
};

extern int32_t program_frustum_loc_vertex (void);
extern void    program_frustum_use (struct program_frustum *values);
