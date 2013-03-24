#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include <GL/gl.h>

#include "world.h"
#include "viewport.h"
#include "layers.h"

static bool
layer_blanktile_full_occlusion (void)
{
	// If the viewport is within world bounds,
	// we will always fully occlude:
	return (viewport_within_world_bounds());
}

static void
layer_blanktile_paint (void)
{
	int tile_left   = viewport_get_tile_left();
	int tile_right  = viewport_get_tile_right() + 1;
	int tile_top    = viewport_get_tile_top();
	int tile_bottom = viewport_get_tile_bottom() + 1;
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

	glColor3f(0.20, 0.20, 0.20);
	glBegin(GL_LINES);

	// Vertical grid lines:
	// Include some extra bounds checks to draw the bottommost line
	// *inside* the tile area instead of 1px outside it.
	// Flip y tile coordinates: tile origin is top left, screen origin is bottom left:
	for (int x = (tile_left < 0) ? 0 : tile_left; x <= tile_right && x <= world_size; x++) {
		double top = (tile_top < 0) ? 0 : tile_top;
		double btm = (tile_bottom >= world_size) ? (double)world_size - 1.0 / 256.0 : tile_bottom;
		double scx = (x >= world_size) ? (double)x - 1.0 / 256.0 : x;
		glVertex2f(cx + scx, cy + (world_size - btm));
		glVertex2f(cx + scx, cy + (world_size - top));
	}
	// Horizontal grid lines:
	for (int y = (tile_top < 0) ? 0 : tile_top; y <= tile_bottom && y <= world_size; y++) {
		double lft = (tile_left < 0) ? 0 : tile_left;
		double rgt = (tile_right >= world_size) ? (double)world_size - 1.0 / 256.0 : tile_right;
		double scy = (y >= world_size) ? (double)y - 1.0 / 256.0 : y;
		glVertex2f(cx + lft, cy + (world_size - scy));
		glVertex2f(cx + rgt, cy + (world_size - scy));
	}
	glEnd();
}

bool
layer_blanktile_create (void)
{
	return layer_register(layer_create(layer_blanktile_full_occlusion, layer_blanktile_paint, NULL, NULL, NULL), 10);
}
