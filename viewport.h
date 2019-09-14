#pragma once

#include <stdbool.h>
#include <stdint.h>

struct viewport {
	uint32_t height;
	uint32_t width;

	// Double-precision internal matrices:
	struct {
		double viewproj[16];
		double model[16];
		double view[16];

		double modelviewproj_origin[16];
		double viewproj_origin[16];
		double view_origin[16];
	} matrix64;

	// Single-precision matrices for use by the GPU:
	struct {
		float modelviewproj_origin[16];
		float viewproj[16];
		float model[16];
		float view[16];
	} matrix32;

	// Double-precision inverse matrices:
	struct {
		double modelview[16];
		double viewproj[16];
		double model[16];
		double view[16];
	} invert64;

	// Single-precision inverse matrices for use by the GPU:
	struct {
		float modelview[16];
		float viewproj[16];
		float model[16];
		float view[16];
	} invert32;

	// Camera position in model space:
	float cam_pos[3];
	float cam_pos_lowbits[3];
};

// The viewport position can be negative if an event happened outside of the
// window, such as a held mousedown drag.
struct viewport_pos {
	int32_t x;
	int32_t y;
};

extern void viewport_destroy (void);
extern bool viewport_unproject (const struct viewport_pos *p, float *lat, float *lon);
extern void viewport_resize (const uint32_t wd, const uint32_t ht);
extern void viewport_paint (void);

extern const struct viewport *viewport_get (void);
extern bool viewport_init (const uint32_t width, const uint32_t height);
