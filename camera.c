#include <math.h>
#include <GL/gl.h>

#include "camera.h"
#include "matrix.h"
#include "vec.h"

static struct camera cam;

static void
matrix_viewproj_update (void)
{
	mat_multiply(cam.matrix.viewproj, cam.matrix.proj, cam.matrix.view);
	mat_invert(cam.matrix.inverse.viewproj, cam.matrix.viewproj);
}

static void
matrix_proj_update (void)
{
	mat_frustum(cam.matrix.proj, cam.view_angle, cam.aspect_ratio, cam.clip.near, cam.clip.far);
	matrix_viewproj_update();
}

static void
matrix_view_update (void)
{
	// Compose view matrix from individual matrices:
	mat_multiply(cam.matrix.view, cam.matrix.tilt, cam.matrix.rotate);
	mat_multiply(cam.matrix.view, cam.matrix.translate, cam.matrix.view);
	mat_invert(cam.matrix.inverse.view, cam.matrix.view);

	// This changes the view-projection matrix:
	matrix_viewproj_update();
}

static void
matrix_translate_update (void)
{
	mat_translate(cam.matrix.translate, 0.0f, 0.0f, -cam.zdist);
	matrix_view_update();
}

static void
matrix_rotate_update (void)
{
	mat_rotate(cam.matrix.rotate, 0.0f, 0.0f, 1.0f, cam.rotate);
	matrix_view_update();
}

static void
matrix_tilt_update (void)
{
	mat_rotate(cam.matrix.tilt, 1.0f, 0.0f, 0.0f, cam.tilt);
	matrix_view_update();
}

const struct camera *
camera_get (void)
{
	return &cam;
}

// Recaculate the projection matrix when screen size changes:
void
camera_projection (const struct viewport *vp)
{
	// This is built around the idea that 1 screen pixel should
	// correspond with 1 texture pixel at 0 degrees tilt angle.

	// Screen aspect ratio:
	cam.aspect_ratio = (float) vp->width / (float) vp->height;

	// Half the screen width in units of 256-pixel tiles:
	float halfwd = (float) vp->width / 2.0f / 256.0f;

	// Horizontal viewing angle:
	cam.view_angle = atanf(halfwd / cam.zdist);

	matrix_proj_update();
}

// Unproject a screen coordinate to get points p1 and p2 in world space:
void
camera_unproject (union vec *p1, union vec *p2, const unsigned int x, const unsigned int y, const unsigned int screen_wd, const unsigned int screen_ht)
{
	// Scale x and y to clip space (-1..1):
	const float sx = (float)x / (screen_wd / 2.0f) - 1.0f;
	const float sy = (float)y / (screen_ht / 2.0f) - 1.0f;

	// Define two points at extremes of z clip space (0..1):
	const union vec a = vec(sx, sy, 0.0f, 1.0f);
	const union vec b = vec(sx, sy, 1.0f, 1.0f);

	// Multiply with the inverse view-projection matrix:
	mat_vec_multiply(p1->elem.f, cam.matrix.inverse.viewproj, a.elem.f);
	mat_vec_multiply(p2->elem.f, cam.matrix.inverse.viewproj, b.elem.f);

	// Divide by w:
	*p1 = vec_div(*p1, vec_1(p1->w));
	*p2 = vec_div(*p2, vec_1(p2->w));
}

void
camera_tilt (const float radians)
{
	cam.tilt += radians;

	// Always keep a small angle to the map:
	if (cam.tilt > 1.4f)
		cam.tilt = 1.4f;

	// If almost vertical, snap to perfectly vertical:
	if (cam.tilt < 0.005f)
		cam.tilt = 0.0f;

	matrix_tilt_update();
}

void
camera_rotate (const float radians)
{
	cam.rotate += radians;
	matrix_rotate_update();
}

bool
camera_init (const struct viewport *vp)
{
	// Initial position in space:
	cam.zdist = 4.0f;

	// Initial attitude:
	cam.tilt   = 0.0f;
	cam.rotate = 0.0f;

	// Clip planes:
	cam.clip.near =   0.5f;
	cam.clip.far  = 100.0f;

	matrix_rotate_update();
	matrix_tilt_update();
	matrix_translate_update();

	// Configure camera projection:
	camera_projection(vp);

	return true;
}
