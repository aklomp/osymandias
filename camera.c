#include <math.h>
#include <GL/gl.h>

#include "camera.h"
#include "matrix.h"
#include "vec.h"

static struct {
	union vec pos;		// position in world space

	float tilt;		// tilt from vertical, in radians
	float rotate;		// rotation along z axis, in radians
	float zdist;		// distance from camera to origin (camera height)
	float aspect_ratio;
	float view_angle;
} cam;

static struct {
	float near;
	float far;
} clip;

static struct {
	float tilt[16];			// Tilt matrix
	float rotate[16];		// Rotation matrix
	float translate[16];		// Translation from origin
	float view[16];			// View matrix
	float proj[16];			// Projection matrix
	float viewproj[16];		// View-projection matrix
	struct {
		float viewproj[16];	// Inverse of view-projection matrix
	} inverse;
} matrix;

static void
matrix_viewproj_update (void)
{
	mat_multiply(matrix.viewproj, matrix.proj, matrix.view);
	mat_invert(matrix.inverse.viewproj, matrix.viewproj);
}

static void
matrix_proj_update (void)
{
	mat_frustum(matrix.proj, cam.view_angle, cam.aspect_ratio, clip.near, clip.far);
	matrix_viewproj_update();
}

static void
matrix_view_update (void)
{
	// Compose view matrix from individual matrices:
	mat_multiply(matrix.view, matrix.tilt, matrix.rotate);
	mat_multiply(matrix.view, matrix.translate, matrix.view);

	// This was reverse-engineered by observing that the values in the
	// third row of the matrix matched the normalized camera position.
	// TODO: derive this properly.
	cam.pos.x = -matrix.view[2]  * matrix.view[14];
	cam.pos.y = -matrix.view[6]  * matrix.view[14];
	cam.pos.z = -matrix.view[10] * matrix.view[14];
	cam.pos.w = 1.0f;

	// This changes the view-projection matrix:
	matrix_viewproj_update();
}

static void
matrix_translate_update (void)
{
	mat_translate(matrix.translate, 0.0f, 0.0f, -cam.zdist);
	matrix_view_update();
}

static void
matrix_rotate_update (void)
{
	mat_rotate(matrix.rotate, 0.0f, 0.0f, 1.0f, cam.rotate);
	matrix_view_update();
}

static void
matrix_tilt_update (void)
{
	mat_rotate(matrix.tilt, 1.0f, 0.0f, 0.0f, cam.tilt);
	matrix_view_update();
}

const float *
camera_mat_viewproj (void)
{
	return matrix.viewproj;
}

const float *
camera_mat_viewproj_inv (void)
{
	return matrix.inverse.viewproj;
}

// Recaculate the projection matrix when screen size changes:
void
camera_projection (const int screen_wd, const int screen_ht)
{
	// This is built around the idea that 1 screen pixel should
	// correspond with 1 texture pixel at 0 degrees tilt angle.

	// Screen aspect ratio:
	cam.aspect_ratio = (float)screen_wd / (float)screen_ht;

	// Half the screen width in units of 256-pixel tiles:
	float halfwd = (float)screen_wd / 2.0f / 256.0f;

	// Horizontal viewing angle:
	cam.view_angle = atanf(halfwd / cam.zdist);

	matrix_proj_update();
}

// Unproject a screen coordinate to get points p1 and p2 in world space:
void
camera_unproject (union vec *p1, union vec *p2, const unsigned int x, const unsigned int y, const unsigned int screen_wd, const unsigned int screen_ht)
{
	// Get inverse view-projection matrix:
	const float *inv = camera_mat_viewproj_inv();

	// Scale x and y to clip space (-1..1):
	const float sx = (float)x / (screen_wd / 2.0f) - 1.0f;
	const float sy = (float)y / (screen_ht / 2.0f) - 1.0f;

	// Define two points at extremes of z clip space (0..1):
	const union vec a = vec(sx, sy, 0.0f, 1.0f);
	const union vec b = vec(sx, sy, 1.0f, 1.0f);

	// Multiply with the inverse view-projection matrix:
	mat_vec_multiply(p1->elem.f, inv, a.elem.f);
	mat_vec_multiply(p2->elem.f, inv, b.elem.f);

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

const float *
camera_pos (void)
{
	return &cam.pos.x;
}

bool
camera_init (void)
{
	// Initial position in space:
	cam.zdist = 4.0f;
	cam.pos = vec(0.0f, 0.0f, cam.zdist, 1.0f);

	// Initial attitude:
	cam.tilt   = 0.0f;
	cam.rotate = 0.0f;

	// Clip planes:
	clip.near =   0.5f;
	clip.far  = 100.0f;

	matrix_rotate_update();
	matrix_tilt_update();
	matrix_translate_update();

	return true;
}
