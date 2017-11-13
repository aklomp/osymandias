#include <stdbool.h>
#include <math.h>
#include <GL/gl.h>
#include <vec/vec.h>

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

float
camera_distance_squared (const union vec *v)
{
	return vec_distance_squared(cam.pos, *v);
}

union vec
camera_distance_squared_quad (const union vec x, const union vec y, const union vec z)
{
	// Difference vectors:
	const union vec dx = vec_square(vec_sub(x, vec_1(cam.pos.x)));
	const union vec dy = vec_square(vec_sub(y, vec_1(cam.pos.y)));
	const union vec dz = vec_square(vec_sub(z, vec_1(cam.pos.z)));

	return vec_add3(dx, dy, dz);
}

union vec
camera_distance_squared_quadedge (const union vec x, const union vec y, const union vec z)
{
	// Calculate the four squared distances from the camera location
	// to the perpendicular foot on each of the edges.
	// Math lifted from:
	// http://mathworld.wolfram.com/Point-LineDistance3-Dimensional.html

	// Get "difference vectors" between edge points:
	// e.g. xdiff = (x1 - x0), (x2 - x1), (x3 - x2), (x0 - x3)
	union vec xdiff = vec_sub(vec_shuffle(x, 1, 2, 3, 0), x);
	union vec ydiff = vec_sub(vec_shuffle(y, 1, 2, 3, 0), y);
	union vec zdiff = vec_sub(vec_shuffle(z, 1, 2, 3, 0), z);

	// Get "difference vectors" between camera position and first edge point:
	union vec xdiffcam = vec_sub(x, vec_1(cam.pos.x));
	union vec ydiffcam = vec_sub(y, vec_1(cam.pos.y));
	union vec zdiffcam = vec_sub(z, vec_1(cam.pos.z));

	// Dot products of (xdiff, ydiff, zdiff) and (xdiffcam, ydiffcam, zdiffcam):
	union vec dots = vec_add3(vec_mul(xdiff, xdiffcam), vec_mul(ydiff, ydiffcam), vec_mul(zdiff, zdiffcam));

	// Squared magnitudes of (xdiff, ydiff, zdiff):
	union vec squared_magnitudes = vec_add3(vec_square(xdiff), vec_square(ydiff), vec_square(zdiff));

	// Find the parameter t for the perpendicular foot on the edge:
	union vec t = vec_div(vec_negate(dots), squared_magnitudes);

	// Clamp: if t < 0, t = 0:
	t = vec_and(t, vec_ge(t, vec_zero()));

	// Clamp: if t > 1, t = 1:
	union vec ones = vec_1(1.0f);
	union vec mask = vec_gt(t, ones);
	t = vec_or(vec_and(ones, mask), vec_and(t, vec_not(mask)));

	// Substitute these t values in the squared-distance formula:
	xdiffcam = vec_square(vec_add(xdiffcam, vec_mul(xdiff, t)));
	ydiffcam = vec_square(vec_add(ydiffcam, vec_mul(ydiff, t)));
	zdiffcam = vec_square(vec_add(zdiffcam, vec_mul(zdiff, t)));

	return vec_add3(xdiffcam, ydiffcam, zdiffcam);
}

bool
camera_visible_quad (const union vec *coords[4])
{
	// Normal of quad is cross product of its diagonals:
	const union vec quad_normal = vec_cross(vec_sub(*coords[2], *coords[0]),
	                                        vec_sub(*coords[3], *coords[1]));

	// The "ray" between camera and a random vertex;
	// if one vertex is visible, they all are:
	const union vec ray = vec_sub(*coords[0], cam.pos);

	// If the dot product of this vector and the quad normal is negative,
	// the quad is not visible:
	if (vec_dot(quad_normal, ray) < 0.0f)
		return false;

	// Project all four vertices through the frustum:
	union vec proj[4];

	mat_vec_multiply(proj[0].elem.f, matrix.viewproj, coords[0]->elem.f);
	mat_vec_multiply(proj[1].elem.f, matrix.viewproj, coords[1]->elem.f);
	mat_vec_multiply(proj[2].elem.f, matrix.viewproj, coords[2]->elem.f);
	mat_vec_multiply(proj[3].elem.f, matrix.viewproj, coords[3]->elem.f);

	// For each of the dimensions x, y and z, if all the projected vertices
	// are < -w or > w at the same time, the quad is not visible:
	//
	//       -w            w
	//        |------------|
	//
	//   0--0    1-----------1   2--2
	//             3----3
	//      4-----------------4
	//
	// Of these tiles, only 0 and 2 are not visible, because all of their
	// points lie past the same outer edge of the clip box.

	if (proj[0].x < -proj[0].w)
	if (proj[1].x < -proj[1].w)
	if (proj[2].x < -proj[2].w)
	if (proj[3].x < -proj[3].w)
		return false;

	if (proj[0].x > proj[0].w)
	if (proj[1].x > proj[1].w)
	if (proj[2].x > proj[2].w)
	if (proj[3].x > proj[3].w)
		return false;

	if (proj[0].y < -proj[0].w)
	if (proj[1].y < -proj[1].w)
	if (proj[2].y < -proj[2].w)
	if (proj[3].y < -proj[3].w)
		return false;

	if (proj[0].y > proj[0].w)
	if (proj[1].y > proj[1].w)
	if (proj[2].y > proj[2].w)
	if (proj[3].y > proj[3].w)
		return false;

	if (proj[0].z < -proj[0].w)
	if (proj[1].z < -proj[1].w)
	if (proj[2].z < -proj[2].w)
	if (proj[3].z < -proj[3].w)
		return false;

	if (proj[0].z > proj[0].w)
	if (proj[1].z > proj[1].w)
	if (proj[2].z > proj[2].w)
	if (proj[3].z > proj[3].w)
		return false;

	// These checks are not sufficient: some tiles (in the planar world)
	// are still marked as visible, even though none of their area is
	// inside the frustum. Will need a proper intersection check for those
	// cases, such as Sutherland-Hodgman polygon clipping.

	return true;
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
