struct program_cursor {
	const float *mat_proj;
	const float *mat_view;
};

struct program *program_cursor (void);
GLint program_cursor_loc_vertex (void);
void program_cursor_use (struct program_cursor *);
