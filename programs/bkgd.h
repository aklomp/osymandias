struct program_bkgd {
	int tex;
};

struct program *program_bkgd (void);
GLint program_bkgd_loc_vertex (void);
GLint program_bkgd_loc_texture (void);
void program_bkgd_use (struct program_bkgd *);
