#include <math.h>
#include <GL/gl.h>

#include "camera.h"
#include "matrix.h"
#include "vec.h"

#define VIEW_ANGLE_MIN	0.15
#define VIEW_ANGLE_MAX	2.40

static struct camera cam;

static void
matrix_proj_update (void)
{
	mat_frustum(cam.matrix.proj, cam.view_angle / 2.0, cam.aspect_ratio, cam.clip.near, cam.clip.far);
	cam.updated.proj = true;
}

static void
matrix_view_update (void)
{
	// Compose view matrix from individual matrices:
	mat_multiply(cam.matrix.view, cam.matrix.tilt, cam.matrix.rotate);
	mat_multiply(cam.matrix.view, cam.matrix.translate, cam.matrix.view);
	mat_multiply(cam.matrix.view, cam.matrix.view, cam.matrix.radius);

	// Obtain the inverse view matrix:
	mat_multiply(cam.invert.view, cam.invert.rotate, cam.invert.tilt);
	mat_multiply(cam.invert.view, cam.invert.view,   cam.invert.translate);
	mat_multiply(cam.invert.view, cam.invert.radius, cam.invert.view);

	// This is the view matrix rendered as if the camera is in the origin,
	// without any translations. By adding the translation back in in the
	// shader, we can increase precision:
	mat_multiply(cam.matrix.view_origin, cam.matrix.tilt, cam.matrix.rotate);
	cam.updated.view = true;
}

static void
matrix_translate_update (void)
{
	mat_translate(cam.matrix.translate, 0.0, 0.0, -cam.distance);
	mat_translate(cam.invert.translate, 0.0, 0.0,  cam.distance);
	matrix_view_update();
}

static void
matrix_rotate_update (void)
{
	mat_rotate(cam.matrix.rotate, 0.0, 0.0, 1.0,  cam.rotate);
	mat_rotate(cam.invert.rotate, 0.0, 0.0, 1.0, -cam.rotate);
	matrix_view_update();
}

static void
matrix_tilt_update (void)
{
	mat_rotate(cam.matrix.tilt, 1.0, 0.0, 0.0,  cam.tilt);
	mat_rotate(cam.invert.tilt, 1.0, 0.0, 0.0, -cam.tilt);
	matrix_view_update();
}

const struct camera *
camera_get (void)
{
	return &cam;
}

void
camera_updated_reset (void)
{
	cam.updated.proj = false;
	cam.updated.view = false;
}

void
camera_set_view_angle (const double radians)
{
	if (radians < VIEW_ANGLE_MIN || radians > VIEW_ANGLE_MAX)
		return;

	cam.view_angle = radians;
	matrix_proj_update();
}

void
camera_set_aspect_ratio (const struct viewport *vp)
{
	cam.aspect_ratio = (double) vp->width / (double) vp->height;
	matrix_proj_update();
}

// Unproject a screen coordinate to get points p1 and p2 in world space:
void
camera_unproject (union vec *p1, union vec *p2, const unsigned int x, const unsigned int y, const struct viewport *vp)
{
	// Scale x and y to clip space (-1..1):
	const float sx = (float) x / (vp->width / 2.0f) - 1.0f;
	const float sy = (float) y / (vp->height / 2.0f) - 1.0f;

	// Define two points at extremes of z clip space (0..1):
	const union vec a = vec(sx, sy, 0.0f, 1.0f);
	const union vec b = vec(sx, sy, 1.0f, 1.0f);

	// Multiply with the inverse view-projection matrix:
	mat_vec32_multiply(p1->elem.f, vp->invert64.viewproj, a.elem.f);
	mat_vec32_multiply(p2->elem.f, vp->invert64.viewproj, b.elem.f);

	// Divide by w:
	*p1 = vec_div(*p1, vec_1(p1->w));
	*p2 = vec_div(*p2, vec_1(p2->w));
}

void
camera_set_tilt (const double radians)
{
	cam.tilt += radians;

	// Always keep a small angle to the map:
	if (cam.tilt > 1.5)
		cam.tilt = 1.5;

	// If almost vertical, snap to perfectly vertical:
	if (cam.tilt < 0.005)
		cam.tilt = 0.0;

	matrix_tilt_update();
}

void
camera_set_rotate (const double radians)
{
	cam.rotate += radians;
	matrix_rotate_update();
}

void
camera_set_distance (const double distance)
{
	cam.distance  = distance;
	cam.clip.near = distance / 2.0;
	matrix_translate_update();
}

bool
camera_init (const struct viewport *vp)
{
	// Initial attitude:
	cam.tilt     = 0.0;
	cam.rotate   = 0.0;
	cam.distance = 1.0;

	// Clip planes:
	cam.clip.near = 0.5f;
	cam.clip.far  = 1.5f;

	// Initialize the static matrix that pushes the camera back from the
	// world origin by one unit radius. The rest of the camera position is
	// calculated from the resulting point:
	mat_translate(cam.matrix.radius, 0.0, 0.0, -1.0);
	mat_translate(cam.invert.radius, 0.0, 0.0,  1.0);

	// Initialize other attitude matrices:
	matrix_rotate_update();
	matrix_tilt_update();
	matrix_translate_update();

	// Set aspect ratio and viewing angle:
	camera_set_aspect_ratio(vp);
	camera_set_view_angle(1.2);

	return true;
}
