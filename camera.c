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
