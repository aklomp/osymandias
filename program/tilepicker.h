#pragma once

struct program_tilepicker {
	const float *cam;
	const float *mat_mv_inv;
	float vp_angle;
	float vp_height;
	float vp_width;
};

extern int  program_tilepicker_loc_vertex (void);
extern void program_tilepicker_use (const struct program_tilepicker *);
