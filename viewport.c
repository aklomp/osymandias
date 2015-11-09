#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>

#include <GL/gl.h>
#include <GL/glu.h>

#include "vector.h"
#include "matrix.h"
#include "autoscroll.h"
#include "world.h"
#include "camera.h"
#include "tilepicker.h"
#include "layers.h"
#include "inlinebin.h"
#include "programs.h"
#include "viewport.h"

static double center_x;		// in world coordinates
static double center_y;		// in world coordinates
static double hold_x;		// Mouse hold/drag at this world coordinate
static double hold_y;
static double frustum_x[4] = { 0.0, 0.0, 0.0, 0.0 };
static double frustum_y[4] = { 0.0, 0.0, 0.0, 0.0 };
static double bbox_x[2] = { 0.0, 0.0 };
static double bbox_y[2] = { 0.0, 0.0 };

// Screen dimensions:
static struct {
	unsigned int width;
	unsigned int height;
} screen;

static int tile_top;
static int tile_left;
static int tile_right;
static int tile_bottom;
static int tile_last;

static double modelview[16];	// OpenGL projection matrices
static double projection[16];

static int viewport_mode;	// see VIEWPORT_MODE_* constants

static int frustum_inside_need_recalc = 1;
static int frustum_coords_need_recalc = 1;

static vec4f frustum_x1;
static vec4f frustum_y1;
static vec4f frustum_x2;
static vec4f frustum_y2;
static vec4f frustum_dvx;
static vec4f frustum_dvy;

static void intersect_lines (double x0, double y0, double x1, double y1, double x2, double y2, double x3, double y3, double *px, double *py);
static bool point_inside_triangle (double tax, double tay, double tbx, double tby, double tcx, double tcy, double ax, double ay);

static void
frustum_changed_location (void)
{
	// Called once when the location of the frustum changes, but no shape
	// change occurs. For instance when the camera autopans over the tiles.
	frustum_coords_need_recalc = 1;
}

static void
frustum_changed_shape (void)
{
	// Called once whenever the frustum changes shape or dimensions.
	frustum_inside_need_recalc = 1;

	// Shape change implies changes in location:
	frustum_changed_location();
}

static void
frustum_inside_recalc (void)
{
	// Pre-calculate some of the common factors for the
	// point_inside_frustum() function. These factors only change when the
	// frustum changes, so for each frame, we only have to calculate them
	// once:

	// The end points of the edge line segments are the start points,
	// shifted by one lane:
	frustum_x2 = vec4f_shuffle(frustum_x1, 1, 2, 3, 0);
	frustum_y2 = vec4f_shuffle(frustum_y1, 1, 2, 3, 0);

	frustum_dvx = frustum_x2 - frustum_x1;
	frustum_dvy = frustum_y2 - frustum_y1;
}

static void
recalc_tile_extents (void)
{
	// Tile coordinates: (0,0) is top left, (tile_last,tile_last) is bottom right:

	tile_last = world_get_size() - 1;
	tile_left = floor(bbox_x[0]);
	tile_right = ceil(bbox_x[1]);
	tile_top = tile_last - floor(bbox_y[1]);
	tile_bottom = tile_last - floor(bbox_y[0]);

	// Clip to world:
	if (tile_left < 0) tile_left = 0;
	if (tile_top < 0) tile_top = 0;
	if (tile_right > tile_last) tile_right = tile_last;
	if (tile_bottom > tile_last) tile_bottom = tile_last;
}

static void
center_set (const double world_x, const double world_y)
{
	unsigned int world_size = world_get_size();

	// In planar mode, clip to plane:
	if (viewport_mode == VIEWPORT_MODE_PLANAR) {
		center_x = (world_x < 0) ? 0.0
		         : (world_x >= world_size) ? world_size
		         : (world_x);
	}
	// In spherical mode, allow horizontal wrap:
	if (viewport_mode == VIEWPORT_MODE_SPHERICAL) {
		center_x = (world_x < 0) ? world_size - world_x
		         : (world_x >= world_size) ? world_x - world_size
		         : (world_x);
	}
	center_y = (world_y < 0) ? 0.0
	         : (world_y >= world_size) ? world_size
	         : (world_y);

	// Frustum size stays the same, but location changed:
	frustum_changed_location();
}

bool
viewport_init (void)
{
	if (!world_init(0))
		return false;

	viewport_mode_set(VIEWPORT_MODE_SPHERICAL);
	center_x = center_y = (double)world_get_size() / 2.0;

	if (!programs_init())
		return false;

	if (!layers_init())
		return false;

	if (!camera_init())
		return false;

	return true;
}

void
viewport_destroy (void)
{
	camera_destroy();
	layers_destroy();
	programs_destroy();
	world_destroy();
	tilepicker_destroy();
}

void
viewport_zoom_in (const int screen_x, const int screen_y)
{
	if (!world_zoom_in())
		return;

	center_x *= 2;
	center_y *= 2;
	layers_zoom(world_get_zoom());

	// Keep same point under mouse cursor:
	if (viewport_mode == VIEWPORT_MODE_PLANAR) {
		int dx = screen_x - screen.width / 2;
		int dy = screen_y - screen.height / 2;
		viewport_scroll(dx, dy);
	}
	// Frustum got smaller:
	frustum_changed_shape();
}

void
viewport_zoom_out (const int screen_x, const int screen_y)
{
	if (!world_zoom_out())
		return;

	center_x /= 2;
	center_y /= 2;
	layers_zoom(world_get_zoom());

	// Keep same point under mouse cursor:
	if (viewport_mode == VIEWPORT_MODE_PLANAR) {
		int dx = screen_x - screen.width / 2;
		int dy = screen_y - screen.height / 2;
		viewport_scroll(-dx / 2, -dy / 2);
	}
	// Frustum got larger:
	frustum_changed_shape();
}

static void
screen_to_world (double sx, double sy, double *wx, double *wy)
{
	// Shortcut: if we know the world is orthogonal, use simpler
	// calculations; this also lets us "bootstrap" the world before
	// the first OpenGL projection:
	if (!camera_is_tilted() && !camera_is_rotated()) {
		*wx = center_x + (sx - (double)screen.width / 2.0) / 256.0;
		*wy = center_y + (sy - (double)screen.height / 2.0) / 256.0;
		return;
	}
	GLdouble wax, way, waz;
	GLdouble wbx, wby, wbz;
	GLint viewport[4] = { 0, 0, screen.width, screen.height };

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
viewport_hold_start (const int screen_x, const int screen_y)
{
	// Save current world coordinates for later reference:
	screen_to_world(screen_x, screen_y, &hold_x, &hold_y);
}

void
viewport_hold_move (const int screen_x, const int screen_y)
{
	// Mouse has moved during a hold; ensure that (hold_x, hold_y)
	// is under the cursor at the given screen position:
	double wx, wy;

	// Point currently under mouse:
	screen_to_world(screen_x, screen_y, &wx, &wy);

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
	screen_to_world(screen.width / 2 + dx, screen.height / 2 + dy, &world_x, &world_y);
	center_set(world_x, world_y);
}

void
viewport_center_at (const int screen_x, const int screen_y)
{
	double world_x, world_y;

	screen_to_world(screen_x, screen_y, &world_x, &world_y);
	center_set(world_x, world_y);
}

void
viewport_tilt (const int dy)
{
	if (dy == 0) return;

	camera_tilt(dy * 0.005f);

	// This warps the frustum:
	frustum_changed_shape();
}

void
viewport_rotate (const int dx)
{
	if (dx == 0) return;

	camera_rotate(dx * -0.005f);

	// Basic shape of frustum stays the same, but it's rotated:
	frustum_changed_location();
}

void
viewport_resize (const unsigned int width, const unsigned int height)
{
	int dx, dy;

	if (autoscroll_update(&dx, &dy))
		viewport_scroll(-dx, -dy);

	screen.width = width;
	screen.height = height;

	// Update camera's projection matrix:
	camera_projection(width, height);

	frustum_changed_shape();

	// Alert layers:
	layers_resize(width, height);

	viewport_render();
}

void
viewport_render (void)
{
	layers_paint();
}

static void
viewport_calc_frustum (void)
{
	// This function is run once when the viewport is setup;
	// values are read out with viewport_get_frustum().

	// NB: screen coordinates: (0,0) is left bottom:
	screen_to_world(0.0,          screen.height, &frustum_x[0], &frustum_y[0]);
	screen_to_world(screen.width, screen.height, &frustum_x[1], &frustum_y[1]);
	screen_to_world(screen.width, 0.0,           &frustum_x[2], &frustum_y[2]);
	screen_to_world(0.0,          0.0,           &frustum_x[3], &frustum_y[3]);

	// Sometimes the far points of the frustum (0 at left top and 1 at right top)
	// will cross the horizon and be flipped; use the property that the bottom two
	// frustum points always lie on one line with the center to check for flip:

	// Frustum points are numbered like this:
	//
	//  0-----------1
	//   \         /
	//    \       /
	//     3-----2
	//
	// 3 and 2 are always the near points, regardless of orientation.

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
	double extend = world_get_size() * ((screen.height > screen.width) ? screen.height : screen.width) / 256.0;

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
	int checked_corners = 0;

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
		// If both points outside world, check for all corner points of
		// the frustum whether they are visible:
		if (!a_inside && !b_inside && !checked_corners)
		{
			// Bottom left:
			if ((bbox_x[0] != 0.0 || bbox_y[0] != 0.0) && point_inside_frustum(0.0, 0.0)) {
				bbox_x[0] = 0.0;
				bbox_y[0] = 0.0;
			}
			// Top left:
			if ((bbox_x[0] != 0.0 || bbox_y[1] != world_size) && point_inside_frustum(0.0, world_size)) {
				bbox_x[0] = 0.0;
				bbox_y[1] = world_size;
			}
			// Top right:
			if ((bbox_x[1] != world_size || bbox_y[1] != world_size) && point_inside_frustum(world_size, world_size)) {
				bbox_x[1] = world_size;
				bbox_y[1] = world_size;
			}
			// Bottom right:
			if ((bbox_x[1] != world_size || bbox_y[0] != 0.0) && point_inside_frustum(world_size, 0.0)) {
				bbox_x[1] = world_size;
				bbox_y[0] = 0.0;
			}
			// No need to do this more than once:
			checked_corners = 1;
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
viewport_gl_setup_world (void)
{
	// Setup viewport:
	glViewport(0, 0, screen.width, screen.height);

	// Setup camera:
	camera_setup();

	// Get matrices for unproject:
	glGetDoublev(GL_MODELVIEW_MATRIX, modelview);
	glGetDoublev(GL_PROJECTION_MATRIX, projection);

	// Must put this logic here because these recalc functions rely on the
	// projection matrices that have just been set up; use control
	// variables to run them only when needed, since this function can be
	// called multiple times per frame.
	if (frustum_coords_need_recalc) {
		viewport_calc_frustum();

		// Floating-point vector versions of above:
		frustum_x1 = (vec4f){ frustum_x[0], frustum_x[1], frustum_x[2], frustum_x[3] };
		frustum_y1 = (vec4f){ frustum_y[0], frustum_y[1], frustum_y[2], frustum_y[3] };

		if (frustum_inside_need_recalc)
		{
			// Relies on the frustum points:
			frustum_inside_recalc();
			frustum_inside_need_recalc = 0;
		}
		// Relies on the precalculations:
		viewport_calc_bbox();
		recalc_tile_extents();
		tilepicker_recalc();
		frustum_coords_need_recalc = 0;
	}
	glDisable(GL_BLEND);

	switch (viewport_mode)
	{
	case VIEWPORT_MODE_PLANAR:
		glDisable(GL_DEPTH_TEST);
		glDepthMask(GL_FALSE);
		break;

	case VIEWPORT_MODE_SPHERICAL:
		glEnable(GL_DEPTH_TEST);
		glDepthMask(GL_TRUE);
		glClear(GL_DEPTH_BUFFER_BIT);
		break;
	}
}

bool
viewport_within_world_bounds (void)
{
	double world_size = (double)world_get_size();

	if (viewport_mode == VIEWPORT_MODE_PLANAR) {
		for (int i = 0; i < 4; i++) {
			if (frustum_x[i] < 0.0 || frustum_x[i] > world_size) return false;
			if (frustum_y[i] < 0.0 || frustum_y[i] > world_size) return false;
		}
		return true;
	}
	if (viewport_mode == VIEWPORT_MODE_SPHERICAL)
	{
		// For now, always redraw background; even if we can calculate
		// that the world occludes the background, perhaps the tile
		// mantle has holes in it that "leak" the background:
		return false;
	}
	return true;
}

unsigned int
viewport_get_wd (void)
{
	return screen.width;
}

unsigned int
viewport_get_ht (void)
{
	return screen.height;
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

int viewport_get_tile_top (void) { return tile_top; }
int viewport_get_tile_left (void) { return tile_left; }
int viewport_get_tile_right (void) { return tile_right; }
int viewport_get_tile_bottom (void) { return tile_bottom; }

void
viewport_mode_set (int mode)
{
	viewport_mode = mode;
}

int
viewport_mode_get (void)
{
	return viewport_mode;
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

bool
point_inside_frustum (float x, float y)
{
	// Use precomputed vectors to quickly determine whether a point is
	// inside the frustum. For all edges of the frustum, calculate the
	// cross product in parallel and check the resulting signs.

	vec4f pvx = vec4f_float(x) - frustum_x1;
	vec4f pvy = vec4f_float(y) - frustum_y1;

	// Calculate cross products:
	vec4f cross = frustum_dvx * pvy - frustum_dvy * pvx;

	// Check if their sign is uniform (assuming clockwise quad):
	vec4i r = (cross >= vec4f_zero());

	if (r[0]) return false;
	if (r[1]) return false;
	if (r[2]) return false;
	if (r[3]) return false;

	return true;
}
