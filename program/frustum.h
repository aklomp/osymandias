#pragma once

#include <stdint.h>

struct program_frustum {
	const float *cam;
	const float *mat_mvp_origin;
	const float *mat_proj;
};

extern int32_t program_frustum_loc_vertex (void);
extern void    program_frustum_use (struct program_frustum *values);
