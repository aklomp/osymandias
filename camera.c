#include <math.h>
#include <GL/gl.h>

#include "camera.h"
#include "matrix.h"
#include "vec.h"

static struct {
	union vec pos;	// position in world space

	float tilt;	// tilt from vertical, in radians
	float rotate;	// rotation along z axis, in radians
	float zdist;	// distance from camera to origin (camera height)
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
		bool viewproj_fresh;	// Whether the inverse is up to date
	} inverse;
} matrix;

// Update view-projection matrix after view or projection changed:
static void
mat_viewproj_update (void)
{
	// Generate the view-projection matrix:
	mat_multiply(matrix.viewproj, matrix.proj, matrix.view);

	// This invalidates the inverse view-projection matrix:
	matrix.inverse.viewproj_fresh = false;
}

// Update view matrix after tilt/rot/trans changed:
static void
mat_view_update (void)
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
	mat_viewproj_update();
}

const float *
camera_mat_viewproj (void)
{
	return matrix.viewproj;
}

const float *
camera_mat_viewproj_inv (void)
{
	// Lazy instantiation:
	if (!matrix.inverse.viewproj_fresh) {
		mat_invert(matrix.inverse.viewproj, matrix.viewproj);
		matrix.inverse.viewproj_fresh = true;
	}

	return matrix.inverse.viewproj;
}

// Recaculate the projection matrix when screen size changes:
void
camera_projection (const int screen_wd, const int screen_ht)
{
	// This is built around the idea that 1 screen pixel should
	// correspond with 1 texture pixel at 0 degrees tilt angle.

	// Screen aspect ratio:
	float aspect = (float)screen_wd / (float)screen_ht;

	// Half the screen width in units of 256-pixel tiles:
	float halfwd = (float)screen_wd / 2.0f / 256.0f;

	// Horizontal viewing angle:
	float angle = atanf(halfwd / cam.zdist);

	// Generate projection matrix:
	mat_frustum(matrix.proj, angle, aspect, clip.near, clip.far);

	// This changes the view-projection matrix:
	mat_viewproj_update();
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

	// Tilt occurs around the x axis:
	mat_rotate(matrix.tilt, 1.0f, 0.0f, 0.0f, cam.tilt);

	// Update view matrix:
	mat_view_update();
}

void
camera_rotate (const float radians)
{
	cam.rotate += radians;

	// Rotate occurs around the z axis:
	mat_rotate(matrix.rotate, 0.0f, 0.0f, 1.0f, cam.rotate);

	// Update view matrix:
	mat_view_update();
}

void
camera_setup (void)
{
	// Upload the projection matrix:
	glMatrixMode(GL_PROJECTION);
	glLoadMatrixf(matrix.proj);

	// Upload the view matrix:
	glMatrixMode(GL_MODELVIEW);
	glLoadMatrixf(matrix.view);
}

bool
camera_is_tilted (void)
{
	return (cam.tilt != 0.0f);
}

bool
camera_is_rotated (void)
{
	return (cam.rotate != 0.0f);
}

const float *
camera_pos (void)
{
	return &cam.pos.x;
}

void
camera_destroy (void)
{
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

	// Initialize tilt, rotate and translate matrices:
	mat_identity(matrix.rotate);
	mat_identity(matrix.tilt);
	mat_translate(matrix.translate, 0.0f, 0.0f, -cam.zdist);

	// Initialize view matrix:
	mat_view_update();

	return true;
}
