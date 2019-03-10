#pragma once

struct program_planar {
	const float *mat_viewproj;
	const float *mat_model;
	float world_size;
};

enum LocPlanar {
	LOC_PLANAR_MAT_VIEWPROJ,
	LOC_PLANAR_MAT_MODEL,
	LOC_PLANAR_WORLD_SIZE,
	LOC_PLANAR_VERTEX,
	LOC_PLANAR_TEXTURE,
};

extern void  program_planar_use (struct program_planar *);
extern GLint program_planar_loc (const enum LocPlanar);
