#pragma once

enum LocBkgd {
	LOC_BKGD_VERTEX,
	LOC_BKGD_TEXTURE,
};

void program_bkgd_use (void);
GLint program_bkgd_loc (const enum LocBkgd);
