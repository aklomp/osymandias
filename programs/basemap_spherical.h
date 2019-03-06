#pragma once

struct program_basemap_spherical {
	const float *mat_viewproj_inv;
	const float *mat_model_inv;
};

GLint program_basemap_spherical_loc_vertex (void);
void program_basemap_spherical_use (struct program_basemap_spherical *);
