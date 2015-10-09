#include <stdbool.h>
#include <math.h>
#include <GL/gl.h>

#include "vector.h"
#include "vector3d.h"
#include "matrix.h"

struct camera
{
	vec4f pos;	// position in world space

	float tilt;	// tilt from vertical, in degrees
	float rot;	// rotation along z axis, in degrees
	float zdist;	// distance from camera to cursor in z units (camera height)

	float clip_near;
	float clip_far;

	float halfwd;	// half of screen width
	float halfht;	// half of screen height

	float htan;	// tan of horizontal viewing angle
	float vtan;	// tan of vertical viewing angle

	vec4f frustum_planes[4];

	float mat_tilt[16];	// Tilt matrix
	float mat_rot[16];	// Rotate matrix
	float mat_trans[16];	// Translation out of origin
	float mat_view[16];	// View matrix
};

static struct camera cam;

// Update view matrix after tilt/rot/trans changed:
static void
mat_view_update (void)
{
	// Compose view matrix from individual matrices:
	mat_multiply(cam.mat_view, cam.mat_tilt, cam.mat_rot);
	mat_multiply(cam.mat_view, cam.mat_trans, cam.mat_view);

	// This was reverse-engineered by observing that the values in the
	// third row of the matrix matched the normalized camera position.
	// TODO: derive this properly.
	cam.pos = (vec4f) {
		cam.mat_view[2]  * -cam.mat_view[14],
		cam.mat_view[6]  * -cam.mat_view[14],
		cam.mat_view[10] * -cam.mat_view[14],
		0.0f
	};

	// TODO: when we also generate our own projection matrix,
	// extract the frustum planes at this point.
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
	mat_rotate(cam.mat_tilt, 1, 0, 0, cam.tilt);

	// Update view matrix:
	mat_view_update();
}

void
camera_rotate (const float radians)
{
	cam.rot += radians;

	// Rotate occurs around the z axis:
	mat_rotate(cam.mat_rot, 0, 0, 1, cam.rot);

	// Update view matrix:
	mat_view_update();
}

static void
extract_frustum_planes (void)
{
	// Source: Gribb & Hartmann,
	// Fast Extraction of Viewing Frustum Planes from the World-View-Projection Matrix

	float m[16];
	glMatrixMode(GL_PROJECTION);
	glPushMatrix();
	glMultMatrixf(cam.mat_view);
	glGetFloatv(GL_PROJECTION_MATRIX, m);
	glPopMatrix();

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

void
camera_setup (const int screen_wd, const int screen_ht)
{
	// The center point (lookat point) is always at (0,0,0); the camera
	// floats around that point on a sphere.

	float halfwd = (float)screen_wd / 512.0f;
	float halfht = (float)screen_ht / 512.0f;

	cam.halfwd = halfwd / cam.zdist;
	cam.halfht = halfht / cam.zdist;

	cam.htan = cam.halfwd / cam.clip_near;
	cam.vtan = cam.halfht / cam.clip_near;

	// This is built around the idea that the screen center is at (0,0) and
	// that 1 pixel of the screen should correspond with 1 texture pixel at
	// 0 degrees tilt angle:
	glFrustum(-cam.halfwd, cam.halfwd, -cam.halfht, cam.halfht, cam.clip_near, cam.clip_far);

	// NB: layers that use this projection need to MANUALLY offset all
	// coordinates with (-center_x, -center_y)! This is necessary because
	// OpenGL uses floats internally, and their precision is not high
	// enough for pixel-perfect placement in far corners of a giant world.
	// So we keep the ortho window close around the origin, where floats
	// are still precise enough, and apply the transformation *manually* in
	// software.
	// Yes, even glTranslated() does not work at the required precision.
	// Yes, even when used on the MODELVIEW matrix.

	// Upload the view matrix:
	glMatrixMode(GL_MODELVIEW);
	glLoadMatrixf(cam.mat_view);

	extract_frustum_planes();
}

float
camera_distance_squared_point (const vec4f a)
{
	// Difference vector:
	vec4f d = a - cam.pos;

	// d[0] * d[0] + d[1] * d[1] + d[2] * d[2]
	return vec4f_hsum(d * d);
}

vec4f
camera_distance_squared_quad (const vec4f x, const vec4f y, const vec4f z)
{
	// Difference vectors:
	vec4f dx = x - vec4f_shuffle(cam.pos, 0, 0, 0, 0);
	vec4f dy = y - vec4f_shuffle(cam.pos, 1, 1, 1, 1);
	vec4f dz = z - vec4f_shuffle(cam.pos, 2, 2, 2, 2);

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
	vec4f xdiffcam = x - vec4f_shuffle(cam.pos, 0, 0, 0, 0);
	vec4f ydiffcam = y - vec4f_shuffle(cam.pos, 1, 1, 1, 1);
	vec4f zdiffcam = z - vec4f_shuffle(cam.pos, 2, 2, 2, 2);

	// Dot products of (xdiff, ydiff, zdiff) and (xdiffcam, ydiffcam, zdiffcam):
	vec4f dots = (xdiff * xdiffcam) + (ydiff * ydiffcam) + (zdiff * zdiffcam);

	// Squared magnitudes of (xdiff, ydiff, zdiff):
	vec4f squared_magnitudes = (xdiff * xdiff) + (ydiff * ydiff) + (zdiff * zdiff);

	// Find the parameter t for the perpendicular foot on the edge:
	vec4f t = -dots / squared_magnitudes;

	// Clamp: if t < 0, t = 0:
	t = (vec4f)((vec4i)t & (t >= vec4f_zero()));

	// Clamp: if t > 1, t = 1:
	vec4f ones = vec4f_float(1.0);
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
camera_visible_quad (const vec4f a, const vec4f b, const vec4f c, const vec4f d)
{
	//  a---b   a = (x, y, z, _)
	//  |   |   b = (x, y, z, _)
	//  d---c   ...

	// Vector between corner points and camera:
	const vec4f a_delta = a - cam.pos;
	const vec4f b_delta = b - cam.pos;
	const vec4f c_delta = c - cam.pos;
	const vec4f d_delta = d - cam.pos;

	// Check sign of cross product of delta vector and quad edges;
	// if any of the signs is positive, the quad is visible.
	// TODO: this is glitchy at globe edges because it uses the flat normal,
	// not the actual sphere normal at the corner points.
	for (;;) {
		if (dot(vector3d_cross(a - b, a - d), a_delta) > 0) break;
		if (dot(vector3d_cross(b - c, b - a), b_delta) > 0) break;
		if (dot(vector3d_cross(c - d, c - b), c_delta) > 0) break;
		if (dot(vector3d_cross(d - a, d - c), d_delta) > 0) break;
		return false;
	}
	// Check each frustum plane, at least one point must be visible:
	for (int i = 0; i < 4; i++) {
		if (point_in_front_of_plane(a, cam.frustum_planes[i])) continue;
		if (point_in_front_of_plane(b, cam.frustum_planes[i])) continue;
		if (point_in_front_of_plane(c, cam.frustum_planes[i])) continue;
		if (point_in_front_of_plane(d, cam.frustum_planes[i])) continue;
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
	return (cam.rot != 0.0f);
}

void
camera_destroy (void)
{
}

bool
camera_init (void)
{
	// Initial position in space:
	cam.zdist = 4;
	cam.pos = (vec4f){ 0, 0, cam.zdist, 0 };

	// Initial attitude:
	cam.tilt = 0;
	cam.rot  = 0;

	// Clip planes:
	cam.clip_near =   1.0;
	cam.clip_far  = 100.0;

	// Initialize tilt, rotate and translate matrices:
	mat_identity(cam.mat_rot);
	mat_identity(cam.mat_tilt);
	mat_translate(cam.mat_trans, 0, 0, -cam.zdist);

	// Initialize view matrix:
	mat_view_update();

	return true;
}
