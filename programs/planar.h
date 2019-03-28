#pragma once

struct program_planar {
	const float *mat_viewproj;
	const float *mat_model;
	int tile_zoom;
	int world_zoom;
};

enum LocPlanar {
	LOC_PLANAR_MAT_VIEWPROJ,
	LOC_PLANAR_MAT_MODEL,
	LOC_PLANAR_TILE_ZOOM,
	LOC_PLANAR_WORLD_ZOOM,
	LOC_PLANAR_VERTEX,
	LOC_PLANAR_TEXTURE,
};

extern void  program_planar_set_tile_zoom (const int zoom);
extern void  program_planar_use (struct program_planar *);
extern GLint program_planar_loc (const enum LocPlanar);
