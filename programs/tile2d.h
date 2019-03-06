#pragma once

struct program_tile2d {
	const float *mat_proj;
};

enum LocCursor {
	LOC_TILE2D_MAT_PROJ,
	LOC_TILE2D_VERTEX,
	LOC_TILE2D_TEXTURE,
};

void program_tile2d_use (struct program_tile2d *);
GLint program_tile2d_loc (const enum LocCursor);
