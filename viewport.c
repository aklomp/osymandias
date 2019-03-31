#include <GL/gl.h>
#include <GL/glu.h>

#include "worlds.h"
#include "camera.h"
#include "tilepicker.h"
#include "layers.h"
#include "programs.h"
#include "viewport.h"

static float hold_x;		// Mouse hold/drag at this world coordinate
static float hold_y;

// Screen dimensions:
static struct {
	unsigned int width;
	unsigned int height;
} screen;

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
screen_to_world (const struct screen_pos *pos, float *wx, float *wy)
{
	const struct coords *center = world_get_center();
	float center_x = center->tile.x;
	float center_y = world_get_size() - center->tile.y;

	union vec p1, p2;

	// Unproject two points at different z index through the
	// view-projection matrix to get two points in world coordinates:
	camera_unproject(&p1, &p2, pos->x, pos->y, screen.width, screen.height);

	// Direction vector is difference between points:
	union vec dir = vec_sub(p2, p1);

	// Get time value to the intersection point with the planar world,
	// where z = 0:
	float t = -p1.z / dir.z;

	// Get x and y coordinates of hit point:
	*wx = p1.x + t * dir.x + center_x;
	*wy = p1.y + t * dir.y + center_y;
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
	float wx, wy;

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
	float world_x, world_y;
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
	float world_x, world_y;

	screen_to_world(pos, &world_x, &world_y);
	center_set(world_x, world_y);
}

void
viewport_paint (void)
{
	// Clear the depth buffer:
	glClear(GL_DEPTH_BUFFER_BIT);
	glEnable(GL_DEPTH_TEST);
	glDepthMask(GL_TRUE);

	// Paint all layers:
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

	// FIXME: only do this when moved?
	tilepicker_recalc();

	glDisable(GL_BLEND);
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
