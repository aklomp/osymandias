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
static struct viewport vp;

static void
center_set (const double world_x, const double world_y)
{
	// FIXME: world_x and world_y should be in tile coordinates:
	world_moveto_tile(world_x, world_get_size() - world_y);
}

void
viewport_destroy (void)
{
	layers_destroy();
	programs_destroy();
	worlds_destroy();
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
	camera_unproject(&p1, &p2, pos->x, pos->y, &vp);

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
		.x = vp.width  / 2 + dx,
		.y = vp.height / 2 + dy,
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
	layers_paint(camera_get());
}

void
viewport_resize (const unsigned int width, const unsigned int height)
{
	vp.width  = width;
	vp.height = height;

	// Setup viewport:
	glViewport(0, 0, vp.width, vp.height);

	// Update camera's projection matrix:
	camera_set_aspect_ratio(&vp);

	// Alert layers:
	layers_resize(&vp);
}

void
viewport_gl_setup_world (void)
{
	// FIXME: only do this when moved?
	tilepicker_recalc();

	glDisable(GL_BLEND);
}

const struct viewport *
viewport_get (void)
{
	return &vp;
}

bool
viewport_init (const uint32_t width, const uint32_t height)
{
	vp.width  = width;
	vp.height = height;

	if (!worlds_init(0, 0.0f, 0.0f))
		return false;

	if (!programs_init())
		return false;

	if (!layers_init(&vp))
		return false;

	if (!camera_init(&vp))
		return false;

	return true;
}
