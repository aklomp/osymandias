struct program_cursor {
	const float *mat_proj;
};

enum LocCursor {
	LOC_CURSOR_MAT_PROJ,
	LOC_CURSOR_VERTEX,
	LOC_CURSOR_TEXTURE,
};

struct program *program_cursor (void);
void program_cursor_use (struct program_cursor *);
GLint program_cursor_loc (const enum LocCursor);
