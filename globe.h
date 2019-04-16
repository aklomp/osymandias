#pragma once

struct globe {

	// Cursor position in radians:
	float lat;
	float lon;

	// Model matrix and components:
	struct {
		float model[16];
		struct {
			float lat[16];
			float lon[16];
		} rotate;
	} matrix;

	// Inverse matrices:
	struct {
		float model[16];
		struct {
			float lat[16];
			float lon[16];
		} rotate;
	} invert;
};

extern const struct globe *globe_get (void);
extern void globe_moveto (const float lat, const float lon);
extern void globe_init (void);
