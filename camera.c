#include <stdbool.h>
#include <math.h>

#include <GL/gl.h>
#include <GL/glu.h>

#include "vector.h"
#include "vector3d.h"

struct camera
{
	vec4f pos;	// position in world space

	float tilt;	// tilt from vertical, in degrees
	float rot;	// rotation along z axis, in degrees
	float zdist;	// distance from camera to cursor in z units (camera height)

	vec4f refx;	// unit vectors in x, y, z directions
	vec4f refy;
	vec4f refz;

	float clip_near;
	float clip_far;

	float halfwd;	// half of screen width
	float halfht;	// half of screen height

	float htan;	// tan of horizontal viewing angle
	float vtan;	// tan of vertical viewing angle

	vec4f frustum_planes[4];
};

static struct camera cam =
{
	.pos = { 0, 0, 4, 0 },

	.tilt = 0,
	.rot = 0,
	.zdist = 4,

	.clip_near = 1.0,
	.clip_far = 100.0,
};

void
camera_tilt (const int dy)
{
	if (dy == 0) return;

	cam.tilt += (float)dy * 0.1;

	if (cam.tilt > 80.0) cam.tilt = 80.0;
	if (cam.tilt < 0.0) cam.tilt = 0.0;

	// Snap to 0.0:
	if (cam.tilt < 0.05) cam.tilt = 0.0;
}

void
camera_rotate (const int dx)
{
	if (dx == 0) return;

	cam.rot += (float)dx * 0.1;

	if (cam.rot < 0.0) cam.rot += 360.0;
	if (cam.rot >= 360.0) cam.rot -= 360.0;
}

static void
extract_frustum_planes (void)
{
	// Source: Gribb & Hartmann,
	// Fast Extraction of Viewing Frustum Planes from the World-View-Projection Matrix

	float m[16];
	glGetFloatv(GL_MODELVIEW_MATRIX, m);

	glMatrixMode(GL_PROJECTION);
	glPushMatrix();
	glMultMatrixf(m);
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

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();

	// gluLookAt() does not work when up vector == view vector (underdefined);
	// use old-fashioned rotate/translate to setup the modelview matrix:
	if (cam.tilt == 0.0)
	{
		float sin, cos;

		glTranslatef(0, 0, -cam.zdist);
		glRotatef(-cam.tilt, 1, 0, 0);
		glRotatef(cam.rot, 0, 0, 1);

		// z reference vector is straight down:
		cam.refz = (vec4f){ 0, 0, -1, 0 };

		// Camera's position is the inverse, since we look at the origin:
		cam.pos = -cam.refz * vec4f_float(cam.zdist);

		// Precalculate sine and cosine of rotation:
		sincosf(cam.rot * M_PI / 180.0f, &sin, &cos);

		// y reference vector is the rotation:
		cam.refy = (vec4f){ sin, cos, 0, 0 };

		// x reference is cross product of the other two:
		cam.refx = vector3d_cross(cam.refz, cam.refy);
	}
	else {
		float lat = -cam.tilt * M_PI / 180.0f;
		float lon = cam.rot * M_PI / 180.0f;
		float sinlat, coslat;
		float sinlon, coslon;

		sincosf(lat, &sinlat, &coslat);
		sincosf(lon, &sinlon, &coslon);

		// Place the camera on a sphere centered around (0,0,0):
		cam.refz = (vec4f){
			-sinlat * sinlon,
			-sinlat * coslon,
			-coslat,
			0
		};
		// Camera's position is the inverse, since we look at the origin:
		cam.pos = -cam.refz * vec4f_float(cam.zdist);

		// The x reference axis is just the rotation in the xy plane:
		cam.refx = (vec4f){ -coslon, sinlon, 0, 0 };

		// Cross x and z to get the y vector:
		cam.refy = vector3d_cross(cam.refx, cam.refz);

		gluLookAt(
			cam.pos[0], cam.pos[1], cam.pos[2],	// Eye point
			0, 0, 0,				// Lookat point (always the origin)
			0, 0, 1					// Up vector
		);
	}
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

static inline bool
vertex_visible (vec4f delta, float *const restrict px, float *const restrict py, float *const restrict pz, int *behind)
{
	// Project vector onto camera unit reference vector;
	// express as scalars in the camera's coordinate system:
	*px = dot(cam.refx, delta);
	*py = dot(cam.refy, delta);
	*pz = dot(cam.refz, delta);

	// Check if point is behind the field of view:
	if ((*behind = (*pz <= cam.clip_near))) {
		return false;
	}
	// Width and height frustum (view pyramid) at original point:
	float wd = *pz * cam.htan;
	float ht = *pz * cam.vtan;

	// Point must be in view pyramid to be visible:
	return (fabsf(*px) <= wd) && (fabsf(*py) <= ht);
}

static inline void
project_quad_on_near_plane (float px[4], float py[4], float pz[4], float cx[5], float cy[5], int *nvert)
{
	*nvert = 0;

	// Check points i and j. A point is "good" if it is in front of the
	// clipping plane, and "bad" if it is behind it.
	// If i is good and j is good, project i;
	// if i is good and j is bad, project i and project i-j onto clip plane;
	// if j is bad and i is good, project i-j onto clip plane;
	// if j is bad and i is bad, do nothing.

	for (int i = 0; i < 4; i++) {
		int j = (i == 3) ? 0 : i + 1;
		bool igood = (pz[i] > cam.clip_near);
		bool jgood = (pz[j] > cam.clip_near);

		if (igood) {
			cx[*nvert] = (px[i] * cam.clip_near) / pz[i];
			cy[*nvert] = (py[i] * cam.clip_near) / pz[i];
			(*nvert)++;
		}
		if (igood ^ jgood) {
			float dx = px[i] - px[j];
			float dy = py[i] - py[j];
			float dz = pz[i] - pz[j];
			cx[*nvert] = px[i] + (dx / dz) * (cam.clip_near - pz[i]);
			cy[*nvert] = py[i] + (dy / dz) * (cam.clip_near - pz[i]);
			(*nvert)++;
		}
	}
}

bool
camera_visible_quad (const vec4f a, const vec4f b, const vec4f c, const vec4f d)
{
	float px[4], py[4], pz[4];
	int behind[4];

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

	// If any of these vertices is directly visible, return:
	if (vertex_visible(a_delta, &px[0], &py[0], &pz[0], &behind[0])) return true;
	if (vertex_visible(b_delta, &px[1], &py[1], &pz[1], &behind[1])) return true;
	if (vertex_visible(c_delta, &px[2], &py[2], &pz[2], &behind[2])) return true;
	if (vertex_visible(d_delta, &px[3], &py[3], &pz[3], &behind[3])) return true;

	// If all four vertices are behind us, quad is invisible:
	if (behind[0] && behind[1] && behind[2] && behind[3]) {
		return false;
	}
	float xproj[5];
	float yproj[5];
	int nvert;

	project_quad_on_near_plane(px, py, pz, xproj, yproj, &nvert);

	// Next, use the separation axis method to check for intersections.
	// first check the intersections in the camera axes:

	float xmin = xproj[0], xmax = xproj[0];
	float ymin = yproj[0], ymax = yproj[0];

	for (int i = 1; i < nvert; i++) {
		xmin = fminf(xmin, xproj[i]);
		xmax = fmaxf(xmax, xproj[i]);
		ymin = fminf(ymin, yproj[i]);
		ymax = fmaxf(ymax, yproj[i]);
	}
	if (xmin >= cam.halfwd || xmax <= -cam.halfwd
	 || ymin >= cam.halfht || ymax <= -cam.halfht) {
		return false;
	}
	// Now try the separation axes method on the projected quad:
	for (int i = 0; i < nvert; i++)
	{
		int j = (i == nvert - 1) ? 0 : i + 1;

		// Perpendicular vector to edge between points i and j:
		float ex = yproj[j] - yproj[i];	// -dy
		float ey = xproj[i] - xproj[j];	//  dx

		// Project all viewport vertices:
		float vproj[4] = {
			ex * -cam.halfwd + ey * -cam.halfht,
			ex *  cam.halfwd + ey * -cam.halfht,
			ex * -cam.halfwd + ey *  cam.halfht,
			ex *  cam.halfwd + ey *  cam.halfht
		};
		float kmin, kmax;
		float vmin = fminf(fminf(vproj[0], vproj[1]), fminf(vproj[2], vproj[3]));
		float vmax = fmaxf(fmaxf(vproj[0], vproj[1]), fmaxf(vproj[2], vproj[3]));

		// Project quad vertices; find projected minimum and maximum:
		for (int k = 0; k < nvert; k++) {
			float kproj = ex * xproj[k] + ey * yproj[k];
			if (k > 0) {
				kmin = fminf(kmin, kproj);
				kmax = fmaxf(kmax, kproj);
			}
			else kmin = kmax = kproj;
		}
		// Check for overlaps:
		if (vmax <= kmin || vmin >= kmax) {
			return false;
		}
	}
	// After exhaustive check, no separation axis found;
	// we conclude that the tile must be visible:
	return true;
}

bool
camera_is_tilted (void)
{
	return (cam.tilt != 0.0);
}

bool
camera_is_rotated (void)
{
	return (cam.rot != 0.0);
}
