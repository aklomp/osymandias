#include <stdbool.h>

#include <GL/gl.h>

#include "../world.h"
#include "../viewport.h"
#include "../layers.h"

#define FOG_END	20.0

static bool
occludes (void)
{
	// If the viewport is within world bounds,
	// we will always fully occlude:
	return (viewport_within_world_bounds());
}

static void
paint (void)
{
	if (viewport_mode_get() != VIEWPORT_MODE_PLANAR) {
		return;
	}
	int world_size  = world_get_size();
	double cx = -viewport_get_center_x();
	double cy = -viewport_get_center_y();

	// Draw to world coordinates:
	viewport_gl_setup_world();

	// Draw a giant quad to the current world size:
	glColor3f(0.12, 0.12, 0.12);
	glBegin(GL_QUADS);
		glVertex2f(cx, cy);
		glVertex2f(cx + world_size, cy);
		glVertex2f(cx + world_size, cy + world_size);
		glVertex2f(cx, cy + world_size);
	glEnd();

	// Setup fog:
	glEnable(GL_FOG);
	glFogfv(GL_FOG_COLOR, (float[]){ 0.12, 0.12, 0.12, 1.0 });
	glHint(GL_FOG_HINT, GL_DONT_CARE);
	glFogi(GL_FOG_MODE, GL_LINEAR);
	glFogf(GL_FOG_DENSITY, 1.0);
	glFogf(GL_FOG_START, 1.0);
	glFogf(GL_FOG_END, FOG_END);

	// Clip tile size to non-fog region:
	int l = -cx - FOG_END - 5;
	int r = -cx + FOG_END + 5;
	int t = -cy + FOG_END + 5;
	int b = -cy - FOG_END - 5;

	l = (l < 0) ? 0 : (l >= world_size) ? world_size : l;
	r = (r < 0) ? 0 : (r >= world_size) ? world_size : r;
	t = (t < 0) ? 0 : (t >= world_size) ? world_size : t;
	b = (b < 0) ? 0 : (b >= world_size) ? world_size : b;

	glColor3f(0.20, 0.20, 0.20);
	glBegin(GL_LINES);

	// Vertical grid lines:
	// Include some extra bounds checks to draw the topmost/rightmost line
	// *inside* the tile area instead of 1px outside it.
	// Flip y tile coordinates: tile origin is top left, screen origin is bottom left:
	for (int x = l; x <= r; x++) {
		double scx = (x == world_size) ? (double)world_size - 1.0 / 256.0 : x;
		double scy = (t == world_size) ? (double)world_size - 1.0 / 256.0 : t;
		glVertex2d(cx + scx, cy + scy);
		glVertex2d(cx + scx, cy + b);
	}
	// Horizontal grid lines:
	for (int y = b; y <= t; y++) {
		double scx = (r == world_size) ? (double)world_size - 1.0 / 256.0 : r;
		double scy = (y == world_size) ? (double)world_size - 1.0 / 256.0 : y;
		glVertex2d(cx + l,   cy + scy);
		glVertex2d(cx + scx, cy + scy);
	}
	glEnd();
	glDisable(GL_FOG);
}

const struct layer *
layer_blanktile (void)
{
	static struct layer layer = {
		.occludes = &occludes,
		.paint    = &paint,
	};

	return &layer;
}
