struct program_solid {
	const float *matrix;
};

struct program *program_solid (void);
GLint program_solid_loc_color (void);
GLint program_solid_loc_vertex (void);
void program_solid_use (struct program_solid *);
