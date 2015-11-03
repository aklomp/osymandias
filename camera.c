#include <stdbool.h>
#include <math.h>
#include <GL/gl.h>

#include "vector.h"
#include "vector3d.h"
#include "matrix.h"

static struct {
	struct vector pos;	// position in world space

	float tilt;		// tilt from vertical, in radians
	float rotate;		// rotation along z axis, in radians
	float zdist;		// distance from camera to origin (camera height)

	vec4f frustum_planes[4];
} cam;

static struct {
	float near;
	float far;
} clip;

static struct {
	float tilt[16];		// Tilt matrix
	float rotate[16];	// Rotate matrix
	float translate[16];	// Translation from origin
	float view[16];		// View matrix
	float proj[16];		// Projection matrix
	float viewproj[16];	// View-projection matrix
} matrix;

static void
extract_frustum_planes (void)
{
	// Source: Gribb & Hartmann,
	// Fast Extraction of Viewing Frustum Planes from the World-View-Projection Matrix

	// Shorthand:
	float *m = matrix.viewproj;

	// The actual calculations, which we can optimize by collecting terms:
	// cam.frustum_planes[0] = (vec4f) { m[3] + m[0], m[7] + m[4], m[11] + m[8], m[15] + m[12] };
	// cam.frustum_planes[1] = (vec4f) { m[3] - m[0], m[7] - m[4], m[11] - m[8], m[15] - m[12] };
	// cam.frustum_planes[2] = (vec4f) { m[3] + m[1], m[7] + m[5], m[11] + m[9], m[15] + m[13] };
	// cam.frustum_planes[3] = (vec4f) { m[3] - m[1], m[7] - m[5], m[11] - m[9], m[15] - m[13] };

	const vec4f c = { m[3], m[7], m[11], m[15] };
	const vec4f u = { m[0], m[4],  m[8], m[12] };
	const vec4f v = { m[1], m[5],  m[9], m[13] };

	cam.frustum_planes[0] = c + u;
	cam.frustum_planes[1] = c - u;
	cam.frustum_planes[2] = c + v;
	cam.frustum_planes[3] = c - v;
}

// Update view-projection matrix after view or projection changed:
static void
mat_viewproj_update (void)
{
	// Generate the view-projection matrix:
	mat_multiply(matrix.viewproj, matrix.proj, matrix.view);

	// Extract the frustum planes from the updated matrix:
	extract_frustum_planes();
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
	cam.pos.x = matrix.view[2]  * matrix.view[14];
	cam.pos.y = matrix.view[6]  * matrix.view[14];
	cam.pos.z = matrix.view[10] * matrix.view[14];
	cam.pos.w = 1.0f;

	// This changes the view-projection matrix:
	mat_viewproj_update();
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

void
camera_tilt (const float radians)
{
	cam.tilt += radians;

	// Always keep a small angle to the map:
	if (cam.tilt < -1.4f)
		cam.tilt = -1.4f;

	// If almost vertical, snap to perfectly vertical:
	if (cam.tilt > -0.005f)
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
camera_distance_squared (const struct vector *v)
{
	return vec_distance_squared(v, &cam.pos);
}

vec4f
camera_distance_squared_quad (const vec4f x, const vec4f y, const vec4f z)
{
	// Difference vectors:
	vec4f dx = x - vec4f_float(cam.pos.x);
	vec4f dy = y - vec4f_float(cam.pos.y);
	vec4f dz = z - vec4f_float(cam.pos.z);

	return (dx * dx) + (dy * dy) + (dz * dz);
}

static inline float
dot (vec4f a, vec4f b)
{
	return vec4f_hsum(a * b);
}

vec4f
camera_distance_squared_quadedge (const vec4f x, const vec4f y, const vec4f z)
{
	// Calculate the four squared distances from the camera location
	// to the perpendicular foot on each of the edges.
	// Math lifted from:
	// http://mathworld.wolfram.com/Point-LineDistance3-Dimensional.html

	// Get "difference vectors" between edge points:
	// e.g. xdiff = (x1 - x0), (x2 - x1), (x3 - x2), (x0 - x3)
	vec4f xdiff = vec4f_shuffle(x, 1, 2, 3, 0) - x;
	vec4f ydiff = vec4f_shuffle(y, 1, 2, 3, 0) - y;
	vec4f zdiff = vec4f_shuffle(z, 1, 2, 3, 0) - z;

	// Get "difference vectors" between camera position and first edge point:
	vec4f xdiffcam = x - vec4f_float(cam.pos.x);
	vec4f ydiffcam = y - vec4f_float(cam.pos.y);
	vec4f zdiffcam = z - vec4f_float(cam.pos.z);

	// Dot products of (xdiff, ydiff, zdiff) and (xdiffcam, ydiffcam, zdiffcam):
	vec4f dots = (xdiff * xdiffcam) + (ydiff * ydiffcam) + (zdiff * zdiffcam);

	// Squared magnitudes of (xdiff, ydiff, zdiff):
	vec4f squared_magnitudes = (xdiff * xdiff) + (ydiff * ydiff) + (zdiff * zdiff);

	// Find the parameter t for the perpendicular foot on the edge:
	vec4f t = -dots / squared_magnitudes;

	// Clamp: if t < 0, t = 0:
	t = (vec4f)((vec4i)t & (t >= vec4f_zero()));

	// Clamp: if t > 1, t = 1:
	vec4f ones = vec4f_float(1.0f);
	vec4i mask = (t <= ones);
	vec4i left = (vec4i)ones & ~mask;
	t = (vec4f)((vec4i)t & mask);
	t = (vec4f)((vec4i)t | left);

	// Substitute these t values in the squared-distance formula:
	xdiffcam += xdiff * t;
	ydiffcam += ydiff * t;
	zdiffcam += zdiff * t;
	xdiffcam *= xdiffcam;
	ydiffcam *= ydiffcam;
	zdiffcam *= zdiffcam;

	return xdiffcam + ydiffcam + zdiffcam;
}

bool
camera_visible_quad (const struct vector *coords[4])
{
	//  a---b   a = (x, y, z, _)
	//  |   |   b = (x, y, z, _)
	//  d---c   ...

	// Convert camera position to vector:
	vec4f vcam = VEC4F(cam.pos);

	// Convert coords to vector:
	const vec4f vpos[4] = {
		VEC4F(*coords[0]),
		VEC4F(*coords[1]),
		VEC4F(*coords[2]),
		VEC4F(*coords[3]),
	};

	// Vectors between corner points and camera:
	const vec4f ray[4] = {
		vpos[0] - vcam,
		vpos[1] - vcam,
		vpos[2] - vcam,
		vpos[3] - vcam,
	};

	// Check sign of cross product of delta vector and quad edges;
	// if any of the signs is positive, the quad is visible.
	// TODO: this is glitchy at globe edges because it uses the flat normal,
	// not the actual sphere normal at the corner points.
	for (;;) {
		if (dot(vector3d_cross(vpos[0] - vpos[1], vpos[0] - vpos[3]), ray[0]) > 0) break;
		if (dot(vector3d_cross(vpos[1] - vpos[2], vpos[1] - vpos[0]), ray[1]) > 0) break;
		if (dot(vector3d_cross(vpos[2] - vpos[3], vpos[2] - vpos[1]), ray[2]) > 0) break;
		if (dot(vector3d_cross(vpos[3] - vpos[0], vpos[3] - vpos[2]), ray[3]) > 0) break;
		return false;
	}
	// Check each frustum plane, at least one point must be visible:
	for (int i = 0; i < 4; i++) {
		if (point_in_front_of_plane(vpos[0], cam.frustum_planes[i])) continue;
		if (point_in_front_of_plane(vpos[1], cam.frustum_planes[i])) continue;
		if (point_in_front_of_plane(vpos[2], cam.frustum_planes[i])) continue;
		if (point_in_front_of_plane(vpos[3], cam.frustum_planes[i])) continue;
		return false;
	}
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
camera_mat_viewproj (void)
{
	return matrix.viewproj;
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
	cam.pos.x = 0.0f;
	cam.pos.y = 0.0f;
	cam.pos.z = cam.zdist;
	cam.pos.w = 1.0f;

	// Initial attitude:
	cam.tilt = 0.0f;
	cam.rotate  = 0.0f;

	// Clip planes:
	clip.near =   1.0f;
	clip.far  = 100.0f;

	// Initialize tilt, rotate and translate matrices:
	mat_identity(matrix.rotate);
	mat_identity(matrix.tilt);
	mat_translate(matrix.translate, 0.0f, 0.0f, cam.zdist);

	// Initialize view matrix:
	mat_view_update();

	return true;
}
