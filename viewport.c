#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>

#include <GL/gl.h>
#include <GL/glu.h>

#include "shaders.h"
#include "autoscroll.h"
#include "world.h"
#include "layers.h"
#include "viewport.h"

static double center_x;		// in world coordinates
static double center_y;		// in world coordinates
static unsigned int screen_wd;	// screen dimension
static unsigned int screen_ht;	// screen dimension
static float view_tilt;		// tilt from vertical, in degrees
static float view_rot;		// rotation along z axis, in degrees
static float view_zdist;	// Distance from camera to cursor in z units (camera height)
static double hold_x;		// Mouse hold/drag at this world coordinate
static double hold_y;
static double frustum_x[4];
static double frustum_y[4];
static double bbox_x[2];
static double bbox_y[2];

static int tile_top;
static int tile_left;
static int tile_right;
static int tile_bottom;
static int tile_last;

static double modelview[16];	// OpenGL projection matrices
static double projection[16];

static void intersect_lines (double x0, double y0, double x1, double y1, double x2, double y2, double x3, double y3, double *px, double *py);
static bool point_inside_triangle (double tax, double tay, double tbx, double tby, double tcx, double tcy, double ax, double ay);

static void
viewport_screen_extents_to_world (double *world_left, double *world_bottom, double *world_right, double *world_top)
{
	viewport_screen_to_world(0, 0, world_left, world_bottom);
	viewport_screen_to_world(screen_wd, screen_ht, world_right, world_top);
}

static void
recalc_tile_extents (void)
{
	// Given center_x, center_y and world_size,
	// get the tile extents shown on screen

	double world_left, world_right, world_top, world_bottom;

	viewport_screen_extents_to_world(&world_left, &world_bottom, &world_right, &world_top);

	// Calculate border tiles. Tile origin is left top, world origin is
	// left bottom. That's why we use world_bottom for tile_top and vice
	// versa:
	tile_last = world_get_size() - 1;
	tile_left = floor(world_left);
	tile_right = ceil(world_right);
	tile_top = floor(world_bottom);
	tile_bottom = ceil(world_top);

	// Clip to world:
	if (tile_left < 0) tile_left = 0;
	if (tile_top < 0) tile_top = 0;
	if (tile_right > tile_last) tile_right = tile_last;
	if (tile_bottom > tile_last) tile_bottom = tile_last;
}

static void
center_set (const double world_x, const double world_y)
{
	// Set center to coordinates, but clip to [0, world_size]

	unsigned int world_size = world_get_size();

	center_x = (world_x < 0) ? 0.0
	         : (world_x >= world_size) ? world_size
	         : (world_x);

	center_y = (world_y < 0) ? 0.0
	         : (world_y >= world_size) ? world_size
	         : (world_y);

	recalc_tile_extents();
}

bool
viewport_init (void)
{
	center_x = center_y = (double)world_get_size() / 2.0;
	recalc_tile_extents();
	view_tilt = 0.0;
	view_rot = 0.0;
	view_zdist = 4;
	return true;
}

void
viewport_destroy (void)
{
}

void
viewport_zoom_in (const int screen_x, const int screen_y)
{
	if (world_zoom_in()) {
		center_x *= 2;
		center_y *= 2;
		layers_zoom();
	}
	// Keep same point under mouse cursor:
	int dx = screen_x - screen_wd / 2;
	int dy = screen_y - screen_ht / 2;
	viewport_scroll(dx, dy);
}

void
viewport_zoom_out (const int screen_x, const int screen_y)
{
	if (world_zoom_out()) {
		center_x /= 2;
		center_y /= 2;
		layers_zoom();
	}
	int dx = screen_x - screen_wd / 2;
	int dy = screen_y - screen_ht / 2;
	viewport_scroll(-dx / 2, -dy / 2);
}

void
viewport_hold_start (const int screen_x, const int screen_y)
{
	// Save current world coordinates for later reference:
	viewport_screen_to_world(screen_x, screen_y, &hold_x, &hold_y);
}

void
viewport_hold_move (const int screen_x, const int screen_y)
{
	// Mouse has moved during a hold; ensure that (hold_x, hold_y)
	// is under the cursor at the given screen position:
	double wx, wy;

	// Point currently under mouse:
	viewport_screen_to_world(screen_x, screen_y, &wx, &wy);

	center_x += hold_x - wx;
	center_y += hold_y - wy;
}

void
viewport_scroll (const int dx, const int dy)
{
	double world_x, world_y;

	// Find out which world coordinate will be in the center of the screen
	// after the offsets have been applied, then set the center to that
	// value:
	viewport_screen_to_world(screen_wd / 2 + dx, screen_ht / 2 + dy, &world_x, &world_y);
	center_set(world_x, world_y);
}

void
viewport_center_at (const int screen_x, const int screen_y)
{
	double world_x, world_y;

	viewport_screen_to_world(screen_x, screen_y, &world_x, &world_y);
	center_set(world_x, world_y);
}

void
viewport_tilt (const int dy)
{
	if (dy == 0) return;

	view_tilt += (double)dy * 0.1;

	if (view_tilt > 80.0) view_tilt = 80.0;
	if (view_tilt < 0.0) view_tilt = 0.0;

	/* Snap to 0.0: */
	if (view_tilt < 0.05) view_tilt = 0.0;
}

void
viewport_rotate (const int dx)
{
	if (dx == 0) return;

	view_rot += (double)dx * 0.1;

	if (view_rot < 0.0) view_rot += 360.0;
	if (view_rot >= 360.0) view_rot -= 360.0;
}

void
viewport_reshape (const unsigned int new_width, const unsigned int new_height)
{
	int dx, dy;

	if (autoscroll_update(&dx, &dy)) {
		viewport_scroll(-dx, -dy);
	}
	screen_wd = new_width;
	screen_ht = new_height;

	recalc_tile_extents();
	viewport_render();
}

void
viewport_render (void)
{
	shaders_init();
	layers_paint();
}

static void
viewport_calc_frustum (void)
{
	// This function is run once when the viewport is setup;
	// values are read out with viewport_get_frustum().

	// NB: screen coordinates: (0,0) is left bottom:
	viewport_screen_to_world(0.0,       screen_ht, &frustum_x[0], &frustum_y[0]);
	viewport_screen_to_world(screen_wd, screen_ht, &frustum_x[1], &frustum_y[1]);
	viewport_screen_to_world(screen_wd, 0.0,       &frustum_x[2], &frustum_y[2]);
	viewport_screen_to_world(0.0,       0.0,       &frustum_x[3], &frustum_y[3]);

	// Sometimes the far points of the frustum (0 at left top and 1 at right top)
	// will cross the horizon and be flipped; use the property that the bottom two
	// frustum points always lie on one line with the center to check for flip:
	for (int i = 0; i < 2; i++)
	{
		// Point across diagonal (antipode):
		int n = (i == 0) ? 2 : 3;

		// Are both points on same side of the center? Then they are flipped
		// (the center by definition should be on the diagonal between the two points):
		if (((center_x < frustum_x[i]) == (center_x < frustum_x[n]))
		 && ((center_y < frustum_y[i]) == (center_y < frustum_y[n]))) {
			goto flip;
		}
	}
	return;

flip:	;
	// If the point was flipped, synthetize the viewport area by taking the
	// flipped vectors and extending them "far enough" in the other
	// direction. This creates a finite viewport that extends well beyond
	// the world area:
	double extend = world_get_size() * ((screen_ht > screen_wd) ? screen_ht : screen_wd) / 256.0;

	frustum_x[0] = frustum_x[3] + extend * (frustum_x[3] - frustum_x[0]);
	frustum_y[0] = frustum_y[3] + extend * (frustum_y[3] - frustum_y[0]);

	frustum_x[1] = frustum_x[2] + extend * (frustum_x[2] - frustum_x[1]);
	frustum_y[1] = frustum_y[2] + extend * (frustum_y[2] - frustum_y[1]);
}

static void
bbox_grow (double x, double y)
{
	if (x < bbox_x[0]) bbox_x[0] = x;
	if (x > bbox_x[1]) bbox_x[1] = x;
	if (y < bbox_y[0]) bbox_y[0] = y;
	if (y > bbox_y[1]) bbox_y[1] = y;
}

static void
viewport_calc_bbox (void)
{
	double world_size = (double)world_get_size();
	double px, py;

	// Reset bounding box:
	bbox_x[0] = bbox_y[0] = world_size + 1;
	bbox_x[1] = bbox_y[1] = -1;

	// Split the frustum into four lines, one for each edge:
	for (int i = 0; i < 4; i++)
	{
		// (ax, ay) and (bx, by) are bounding points of the edge:
		double ax = frustum_x[i];
		double ay = frustum_y[i];
		double bx = frustum_x[(i == 3) ? 0 : i + 1];
		double by = frustum_y[(i == 3) ? 0 : i + 1];

		// Is this point inside the world plane?
		int a_inside = (ax >= 0.0 && ax <= world_size && ay >= 0.0 && ay <= world_size);
		int b_inside = (bx >= 0.0 && bx <= world_size && by >= 0.0 && by <= world_size);

		// If line is completely inside world, expand our bounding box to this segment:
		if (a_inside && b_inside) {
			bbox_grow(ax, ay);
			bbox_grow(bx, by);
			continue;
		}
		// Always grow bounding box by the "inner" point:
		if (a_inside) {
			bbox_grow(ax, ay);
		}
		if (b_inside) {
			bbox_grow(bx, by);
		}
		// If both points outside world, create a triangle with the
		// next and the previous points. Check if corner points of the
		// world are inside these triangles:
		if (!a_inside && !b_inside)
		{
			for (int j = 0; j < 2; j++)
			{
				// Which other point to form triangle with?
				int n = (j == 0)
					? (i >= 2) ? i - 2 : i + 2
					: (i == 0) ? 3 : i - 1;

				double cx = frustum_x[n];
				double cy = frustum_y[n];

				// The triangle (a, b, c) is formed with the outside rib as one edge
				// and a neighbouring point as the peak. Check for all world corner
				// points whether they are inside the triangle; if so, they are
				// visible:

				// Bottom left:
				if ((bbox_x[0] != 0.0 || bbox_y[0] != 0.0) && point_inside_triangle(ax, ay, bx, by, cx, cy, 0.0, 0.0)) {
					bbox_x[0] = 0.0;
					bbox_y[0] = 0.0;
				}
				// Top left:
				if ((bbox_x[0] != 0.0 || bbox_y[1] != world_size) && point_inside_triangle(ax, ay, bx, by, cx, cy, 0.0, world_size)) {
					bbox_x[0] = 0.0;
					bbox_y[1] = world_size;
				}
				// Top right:
				if ((bbox_x[1] != world_size || bbox_y[1] != world_size) && point_inside_triangle(ax, ay, bx, by, cx, cy, world_size, world_size)) {
					bbox_x[1] = world_size;
					bbox_y[1] = world_size;
				}
				// Bottom right:
				if ((bbox_x[1] != world_size || bbox_y[0] != 0.0) && point_inside_triangle(ax, ay, bx, by, cx, cy, world_size, 0.0)) {
					bbox_x[1] = world_size;
					bbox_y[0] = 0.0;
				}
			}
		}
		// Intersect this line with all four edges of the world:

		// Left vertical edge:
		if ((ax < 0.0 && bx >= 0.0) || (bx < 0.0 && ax >= 0.0)) {
			intersect_lines(ax, ay, bx, by, 0.0, 0.0, 0.0, world_size, &px, &py);
			if (py >= 0.0 && py <= world_size) bbox_grow(0.0, py);
		}
		// Right vertical edge:
		if ((ax < world_size && bx >= world_size) || (bx < world_size && ax >= world_size)) {
			intersect_lines(ax, ay, bx, by, world_size, 0.0, world_size, world_size, &px, &py);
			if (py >= 0.0 && py <= world_size) bbox_grow(world_size, py);
		}
		// Bottom horizontal edge:
		if ((ay < 0.0 && by >= 0.0) || (by < 0.0 && ay >= 0.0)) {
			intersect_lines(ax, ay, bx, by, 0.0, 0.0, world_size, 0.0, &px, &py);
			if (px >= 0.0 && px <= world_size) bbox_grow(px, 0.0);
		}
		// Top horizontal edge:
		if ((ay < world_size && by >= world_size) || (by < world_size && ay >= world_size)) {
			intersect_lines(ax, ay, bx, by, 0.0, world_size, world_size, world_size, &px, &py);
			if (px >= 0.0 && px <= world_size) bbox_grow(px, world_size);
		}
	}
}

void
viewport_gl_setup_screen (void)
{
	// Setup the OpenGL frustrum to map 1:1 to screen coordinates,
	// with the origin at left bottom. Handy for drawing items that
	// are statically positioned relative to the screen, such as the
	// background and the cursor.

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho(0, screen_wd, 0, screen_ht, 0, 1);
	glViewport(0, 0, screen_wd, screen_ht);

	// Slight translation to snap line artwork to pixels:
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	glTranslatef(0.375, 0.375, 0.0);

	glDisable(GL_BLEND);
	glDisable(GL_DEPTH_TEST);
	glDepthMask(GL_FALSE);
}

void
viewport_gl_setup_world (void)
{
	// Setup the OpenGL frustrum to map screen to world coordinates,
	// with the origin at left bottom. This is a different convention
	// from the one OSM uses for its tiles: origin at left top. But it
	// makes tile and texture handling much easier (no vertical flips).

	double halfwd = (double)screen_wd / 512.0;
	double halfht = (double)screen_ht / 512.0;

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glViewport(0, 0, screen_wd, screen_ht);

	// Pixel snap offset, ensures proper pixel rounding:
	glTranslatef(0.375 / 256.0, 0.375 / 256.0, 0.0);

	// This is built around the idea that the screen center is at (0,0) and
	// that 1 pixel of the screen should correspond with 1 texture pixel at
	// 0 degrees tilt angle:
	glFrustum(-halfwd / view_zdist, halfwd / view_zdist, -halfht / view_zdist, halfht / view_zdist, 1.0, 100.0);

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

	glTranslatef(0.0, 0.0, -view_zdist);
	glRotatef(-view_tilt, 1.0, 0.0, 0.0);
	glRotatef(view_rot, 0.0, 0.0, 1.0);

	glGetDoublev(GL_MODELVIEW_MATRIX, modelview);
	glGetDoublev(GL_PROJECTION_MATRIX, projection);

	viewport_calc_frustum();
	viewport_calc_bbox();

	glDisable(GL_BLEND);
	glDisable(GL_DEPTH_TEST);
	glDepthMask(GL_FALSE);
}

bool
viewport_within_world_bounds (void)
{
	double world_left, world_bottom, world_right, world_top;
	unsigned int world_size = world_get_size();

	viewport_screen_extents_to_world(&world_left, &world_bottom, &world_right, &world_top);

	// Does part of the viewport show an area outside of the world?
	if (world_left < 0) return false;
	if (world_bottom < 0) return false;
	if (world_right >= world_size) return false;
	if (world_top >= world_size) return false;
	return true;
}

unsigned int
viewport_get_wd (void)
{
	return screen_wd;
}

unsigned int
viewport_get_ht (void)
{
	return screen_ht;
}

double
viewport_get_center_x (void)
{
	return center_x;
}

double
viewport_get_center_y (void)
{
	return center_y;
}

float
viewport_get_rot (void)
{
	return view_rot;
}

int viewport_get_tile_top (void) { return tile_top; }
int viewport_get_tile_left (void) { return tile_left; }
int viewport_get_tile_right (void) { return tile_right; }
int viewport_get_tile_bottom (void) { return tile_bottom; }


void
viewport_screen_to_world (double sx, double sy, double *wx, double *wy)
{
	// Shortcut: if we know the world is orthogonal, use simpler
	// calculations; this also lets us "bootstrap" the world before
	// the first OpenGL projection:
	if (view_tilt == 0.0 && view_rot == 0.0) {
		*wx = center_x + (sx - (double)screen_wd / 2.0) / 256.0;
		*wy = center_y + (sy - (double)screen_ht / 2.0) / 256.0;
		return;
	}
	GLdouble wax, way, waz;
	GLdouble wbx, wby, wbz;
	GLint viewport[4] = { 0, 0, screen_wd, screen_ht };

	// Project two points at different z index to get a vector in world space:
	gluUnProject(sx, sy, 0.75, modelview, projection, viewport, &wax, &way, &waz);
	gluUnProject(sx, sy, 1.0, modelview, projection, viewport, &wbx, &wby, &wbz);

	// Project this vector onto the world plane to get the world coordinates;
	//
	//     (p0 - l0) . n
	// t = -------------
	//        l . n
	//
	// n = normal vector of plane = (0, 0, 1);
	// p0 = point in plane, (0, 0, 0);
	// l0 = point on line, (wax, way, waz);
	// l = point on line, (wbx, wby, wbz);
	//
	// This can be rewritten as:
	double t = -waz / wbz;

	*wx = wax + t * (wbx - wax) + center_x;
	*wy = way + t * (wby - way) + center_y;
}

void
viewport_world_to_screen (double wx, double wy, double *sx, double *sy)
{
	if (view_tilt == 0.0 && view_rot == 0.0) {
		*sx = (double)screen_wd / 2.0 + (wx - center_x) * 256.0;
		*sy = (double)screen_ht / 2.0 + (wy - center_y) * 256.0;
		return;
	}
	GLdouble sz;
	GLint viewport[4] = { 0, 0, screen_wd, screen_ht };

	gluProject(wx - center_x, wy - center_y, 0.0, modelview, projection, viewport, sx, sy, &sz);
}

void
viewport_get_frustum (double **wx, double **wy)
{
	*wx = frustum_x;
	*wy = frustum_y;
}

void
viewport_get_bbox (double **bx, double **by)
{
	*bx = bbox_x;
	*by = bbox_y;
}

static void
intersect_lines (double x0, double y0, double x1, double y1, double x2, double y2, double x3, double y3, double *px, double *py)
{
	double d = (x0 - x1) * (y2 - y3) - (y0 - y1) * (x2 - x3);
	double e = x0 * y1 - y0 * x1;
	double f = x2 * y3 - y2 * x3;

	*px = (e * (x2 - x3) - f * (x0 - x1)) / d;
	*py = (e * (y2 - y3) - f * (y0 - y1)) / d;
}

static bool
point_inside_triangle (double tax, double tay, double tbx, double tby, double tcx, double tcy, double ax, double ay)
{
	// Compute vectors:
	double vx[3] = { tcx - tax, tbx - tax, ax - tax };
	double vy[3] = { tcy - tay, tby - tay, ay - tay };

	// Dot products:
	double dot00 = vx[0] * vx[0] + vy[0] * vy[0];
	double dot01 = vx[0] * vx[1] + vy[0] * vy[1];
	double dot02 = vx[0] * vx[2] + vy[0] * vy[2];
	double dot11 = vx[1] * vx[1] + vy[1] * vy[1];
	double dot12 = vx[1] * vx[2] + vy[1] * vy[2];

	double inv = 1.0 / (dot00 * dot11 - dot01 * dot01);

	// Barycentric coordinates:
	double u, v;

	if ((u = (dot11 * dot02 - dot01 * dot12) * inv) < 0.0) {
		return false;
	}
	if ((v = (dot00 * dot12 - dot01 * dot02) * inv) < 0.0) {
		return false;
	}
	return (u + v < 1.0);
}
