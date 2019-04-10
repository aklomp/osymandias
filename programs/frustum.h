#pragma once

struct program_frustum {
	const float *mat_proj;
	const float *mat_frustum;
	const float *mat_model;
	int world_size;
	const float *camera;
};

GLint program_frustum_loc_vertex (void);
void program_frustum_use (struct program_frustum *values);
