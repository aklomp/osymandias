#pragma once

#include <stdbool.h>

#include <vec/vec.h>

#include "viewport.h"

struct camera {

	// Tilt from vertical, in radians:
	double tilt;

	// Rotation along z axis, in radians:
	double rotate;

	// Distance from camera to origin (camera height):
	double distance;

	// Horizontal view angle in radians:
	double view_angle;

	// Viewport aspect ratio:
	double aspect_ratio;

	// Zoom level:
	int zoom;

	struct {
		float near;
		float far;
	} clip;

	// Various view and projection matrices:
	struct {
		double tilt[16];
		double rotate[16];
		double translate[16];
		double radius[16];
		double proj[16];
		double view[16];
		double view_origin[16];
	} matrix;

	// Inverse matrices:
	struct {
		double tilt[16];
		double rotate[16];
		double translate[16];
		double radius[16];
		double view[16];
	} invert;

	// Matrix update flags:
	struct {
		bool proj;
		bool view;
	} updated;
};

extern const struct camera *camera_get (void);
extern void camera_updated_reset (void);
extern void camera_unproject (union vec *, union vec *, const unsigned int x, const unsigned int y, const struct viewport *vp);
extern void camera_set_aspect_ratio (const struct viewport *vp);
extern void camera_set_view_angle (const double radians);
extern void camera_set_rotate (const double radians);
extern void camera_set_tilt (const double radians);
extern void camera_set_distance (const double distance);
extern bool camera_init (const struct viewport *vp);
