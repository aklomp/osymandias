#pragma once

struct program_tilepicker {
	const float *mat_viewproj_inv;
	const float *mat_model_inv;
};

extern int  program_tilepicker_loc_vertex (void);
extern void program_tilepicker_use (const struct program_tilepicker *);
