#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>

#include <GL/gl.h>
#include <GL/glu.h>

#include "vector.h"
#include "matrix.h"
#include "worlds.h"
#include "camera.h"
#include "tilepicker.h"
#include "layers.h"
#include "inlinebin.h"
#include "programs.h"
#include "viewport.h"

static double hold_x;		// Mouse hold/drag at this world coordinate
static double hold_y;

// Screen dimensions:
static struct {
	unsigned int width;
	unsigned int height;
} screen;

static double modelview[16];	// OpenGL projection matrices
static double projection[16];

static void
center_set (const double world_x, const double world_y)
{
	// FIXME: world_x and world_y should be in tile coordinates:
	world_moveto_tile(world_x, world_get_size() - world_y);
}

bool
viewport_init (void)
{
	if (!worlds_init(0, 0.0f, 0.0f))
		return false;

	world_set(WORLD_SPHERICAL);

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
	worlds_destroy();
	tilepicker_destroy();
}

void
viewport_zoom_in (const struct screen_pos *pos)
{
	if (!world_zoom_in())
		return;

	layers_zoom(world_get_zoom());

	// Keep same point under mouse cursor:
	if (world_get() == WORLD_PLANAR) {
		int dx = pos->x - screen.width / 2;
		int dy = pos->y - screen.height / 2;
		viewport_scroll(dx, dy);
	}
}

void
viewport_zoom_out (const struct screen_pos *pos)
{
	if (!world_zoom_out())
		return;

	layers_zoom(world_get_zoom());

	// Keep same point under mouse cursor:
	if (world_get() == WORLD_PLANAR) {
		int dx = pos->x - screen.width / 2;
		int dy = pos->y - screen.height / 2;
		viewport_scroll(-dx / 2, -dy / 2);
	}
}

static void
screen_to_world (const struct screen_pos *pos, double *wx, double *wy)
{
	const struct coords *center = world_get_center();
	float center_x = center->tile.x;
	float center_y = world_get_size() - center->tile.y;

	// Shortcut: if we know the world is orthogonal, use simpler
	// calculations; this also lets us "bootstrap" the world before
	// the first OpenGL projection:
	if (!camera_is_tilted() && !camera_is_rotated()) {
		*wx = center_x + (pos->x - (double)screen.width / 2.0) / 256.0;
		*wy = center_y + (pos->y - (double)screen.height / 2.0) / 256.0;
		return;
	}
	GLdouble wax, way, waz;
	GLdouble wbx, wby, wbz;
	GLint viewport[4] = { 0, 0, screen.width, screen.height };

	// Project two points at different z index to get a vector in world space:
	gluUnProject(pos->x, pos->y, 0.75, modelview, projection, viewport, &wax, &way, &waz);
	gluUnProject(pos->x, pos->y, 1.0, modelview, projection, viewport, &wbx, &wby, &wbz);

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
viewport_hold_start (const struct screen_pos *pos)
{
	// Save current world coordinates for later reference:
	screen_to_world(pos, &hold_x, &hold_y);
}

void
viewport_hold_move (const struct screen_pos *pos)
{
	// Mouse has moved during a hold; ensure that (hold_x, hold_y)
	// is under the cursor at the given screen position:
	double wx, wy;

	// Point currently under mouse:
	screen_to_world(pos, &wx, &wy);

	const struct coords *center = world_get_center();
	float center_x = center->tile.x;
	float center_y = world_get_size() - center->tile.y;

	center_set(center_x + hold_x - wx, center_y + hold_y - wy);
}

void
viewport_scroll (const int dx, const int dy)
{
	double world_x, world_y;
	struct screen_pos pos = {
		.x = screen.width  / 2 + dx,
		.y = screen.height / 2 + dy,
	};

	// Find out which world coordinate will be in the center of the screen
	// after the offsets have been applied, then set the center to that
	// value:
	screen_to_world(&pos, &world_x, &world_y);
	center_set(world_x, world_y);
}

void
viewport_center_at (const struct screen_pos *pos)
{
	double world_x, world_y;

	screen_to_world(pos, &world_x, &world_y);
	center_set(world_x, world_y);
}

void
viewport_paint (void)
{
	layers_paint();
}

void
viewport_resize (const unsigned int width, const unsigned int height)
{
	screen.width = width;
	screen.height = height;

	// Setup viewport:
	glViewport(0, 0, screen.width, screen.height);

	// Update camera's projection matrix:
	camera_projection(width, height);

	// Alert layers:
	layers_resize(width, height);
}

void
viewport_gl_setup_world (void)
{
	// Setup camera:
	camera_setup();

	// Get matrices for unproject:
	glGetDoublev(GL_MODELVIEW_MATRIX, modelview);
	glGetDoublev(GL_PROJECTION_MATRIX, projection);

	// FIXME: only do this when moved?
	tilepicker_recalc();

	glDisable(GL_BLEND);

	switch (world_get())
	{
	case WORLD_PLANAR:
		glDisable(GL_DEPTH_TEST);
		glDepthMask(GL_FALSE);
		break;

	case WORLD_SPHERICAL:
		glEnable(GL_DEPTH_TEST);
		glDepthMask(GL_TRUE);
		glClear(GL_DEPTH_BUFFER_BIT);
		break;
	}
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
