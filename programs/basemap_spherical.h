struct program_basemap_spherical {
	const float *mat_viewproj_inv;
	const float *mat_model_inv;
};

struct program *program_basemap_spherical (void);
GLint program_basemap_spherical_loc_vertex (void);
void program_basemap_spherical_use (struct program_basemap_spherical *);
