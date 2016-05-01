enum LocBkgd {
	LOC_BKGD_VERTEX,
	LOC_BKGD_TEXTURE,
};

struct program *program_bkgd (void);
void program_bkgd_use (void);
GLint program_bkgd_loc (const enum LocBkgd);
