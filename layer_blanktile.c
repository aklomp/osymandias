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

	// Draw to world coordinates:
	viewport_gl_setup_world();

	// Draw a giant quad to the current world size:
	glColor3f(0.12, 0.12, 0.12);
	glBegin(GL_QUADS);
		glVertex2f(0.0, 0.0);
		glVertex2f(world_size, 0.0);
		glVertex2f(world_size, world_size);
		glVertex2f(0.0, world_size);
	glEnd();

	glColor3f(0.20, 0.20, 0.20);
	glBegin(GL_LINES);

	// Vertical grid lines:
	// Include some extra bounds checks to draw the bottommost line
	// *inside* the tile area instead of 1px outside it:
	for (int x = (tile_left < 0) ? 0 : tile_left; x <= tile_right && x * 256 <= world_size; x++) {
		int top = (tile_top < 0) ? 0 : tile_top * 256;
		int btm = (tile_bottom * 256 >= world_size) ? world_size - 1 : tile_bottom * 256;
		int scx = (x * 256 >= world_size) ? x * 256 - 1 : x * 256;
		glVertex2f(scx, btm);
		glVertex2f(scx, top);
	}
	// Horizontal grid lines:
	for (int y = (tile_top < 0) ? 0 : tile_top; y <= tile_bottom && y * 256 <= world_size; y++) {
		int lft = (tile_left < 0) ? 0 : tile_left * 256;
		int rgt = (tile_right * 256 >= world_size) ? world_size - 1 : tile_right * 256;
		int scy = (y * 256 >= world_size) ? y * 256 - 1 : y * 256;
		glVertex2f(lft, scy);
		glVertex2f(rgt, scy);
	}
	glEnd();
}

bool
layer_blanktile_create (void)
{
	return layer_register(layer_create(layer_blanktile_full_occlusion, layer_blanktile_paint, NULL, NULL, NULL), 10);
}
