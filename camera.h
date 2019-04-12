#pragma once

#include <stdbool.h>

#include <vec/vec.h>

struct camera {

	// Tilt from vertical, in radians:
	float tilt;

	// Rotation along z axis, in radians:
	float rotate;

	// Fistance from camera to origin (camera height):
	float zdist;

	// Horizontal view angle in radians:
	float view_angle;

	// Viewport aspect ratio:
	float aspect_ratio;

	struct {
		float near;
		float far;
	} clip;

	struct {
		float tilt[16];
		float rotate[16];
		float translate[16];
		float view[16];
		float proj[16];
		float viewproj[16];
		struct {
			float viewproj[16];
			float view[16];
		} inverse;
	} matrix;
};

extern const struct camera *camera_get (void);
extern void camera_projection (const int screen_wd, const int screen_ht);
extern void camera_unproject (union vec *, union vec *, const unsigned int x, const unsigned int y, const unsigned int screen_wd, const unsigned int screen_ht);
extern void camera_rotate (const float radians);
extern void camera_tilt (const float radians);
extern bool camera_init (void);
