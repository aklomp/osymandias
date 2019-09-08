#pragma once

#include <stdbool.h>

#include <vec/vec.h>

#include "viewport.h"

struct camera {

	// Tilt from vertical, in radians:
	float tilt;

	// Rotation along z axis, in radians:
	float rotate;

	// Distance from camera to origin (camera height):
	float distance;

	// Horizontal view angle in radians:
	float view_angle;

	// Viewport aspect ratio:
	float aspect_ratio;

	// Zoom level:
	int zoom;

	struct {
		float near;
		float far;
	} clip;

	// Various view and projection matrices:
	struct {
		float tilt[16];
		float rotate[16];
		float translate[16];
		float radius[16];
		float viewproj[16];
		float proj[16];
		float view[16];
	} matrix;

	// Inverse matrices:
	struct {
		float tilt[16];
		float rotate[16];
		float translate[16];
		float radius[16];
		float viewproj[16];
		float view[16];
	} invert;

	// Matrix update flags:
	struct {
		bool proj;
		bool view;
		bool viewproj;
	} updated;
};

extern const struct camera *camera_get (void);
extern void camera_updated_reset (void);
extern void camera_unproject (union vec *, union vec *, const unsigned int x, const unsigned int y, const struct viewport *vp);
extern void camera_set_aspect_ratio (const struct viewport *vp);
extern void camera_set_view_angle (const float radians);
extern void camera_set_rotate (const float radians);
extern void camera_set_tilt (const float radians);
extern void camera_set_distance (const float distance);
extern bool camera_init (const struct viewport *vp);
