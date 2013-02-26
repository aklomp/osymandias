#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <GL/gl.h>

#include "shaders.h"
#include "autoscroll.h"
#include "world.h"
#include "layers.h"
#include "viewport.h"

static unsigned int center_x;	// in world coordinates
static unsigned int center_y;	// in world coordinates
static unsigned int screen_wd;	// screen dimension
static unsigned int screen_ht;	// screen dimension

static int tile_top;
static int tile_left;
static int tile_right;
static int tile_bottom;
static int tile_last;

static void viewport_gl_setup (void);

static void
recalc_tile_extents (void)
{
	// Given center_x, center_y and world_size,
	// get the tile extents shown on screen

	int screen_left = center_x - screen_wd / 2;
	int screen_right = center_x + screen_wd / 2;
	int screen_top = center_y - screen_ht / 2;
	int screen_bottom = center_y + screen_ht / 2;

	// Calculate border tiles, keep 1 tile margin:
	tile_last = world_get_size() / 256 - 1;
	tile_left = screen_left / 256 - 1;
	tile_right = screen_right / 256 + 1;
	tile_top = screen_top / 256 - 1;
	tile_bottom = screen_bottom / 256 + 1;

	// Clip to world:
	if (tile_left < 0) tile_left = 0;
	if (tile_top < 0) tile_top = 0;
	if (tile_right > tile_last) tile_right = tile_last;
	if (tile_bottom > tile_last) tile_bottom = tile_last;
}

static void
center_add_delta (unsigned int *const center, const int delta)
{
	// Add delta to center coordinates, but clip to [0, world_size]:

	unsigned int world_size = world_get_size();

	if (delta == 0) {
		return;
	}
	if (delta < 0) {
		unsigned int abs_delta = (unsigned int)(-delta);
		if (abs_delta <= *center) {
			*center -= abs_delta;
		}
		else {
			*center = 0;
		}
	}
	else {
		if (*center + delta <= world_size) {
			*center += delta;
		}
		else {
			*center = world_size;
		}
	}
}

bool
viewport_init (void)
{
	center_x = center_y = world_get_size() / 2;
	recalc_tile_extents();
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
	viewport_scroll(-dx, -dy);
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
	viewport_scroll(dx / 2, dy / 2);
}

void
viewport_scroll (const int dx, const int dy)
{
	center_add_delta(&center_x, -dx);
	center_add_delta(&center_y, -dy);
	recalc_tile_extents();
}

void
viewport_center_at (const int screen_x, const int screen_y)
{
	int dx = screen_wd / 2 - screen_x;
	int dy = screen_ht / 2 - screen_y;

	viewport_scroll(dx, dy);
}

void
viewport_reshape (const unsigned int new_width, const unsigned int new_height)
{
	int dx;
	int dy;

	if (autoscroll_update(&dx, &dy)) {
		viewport_scroll(dx, dy);
	}
	screen_wd = new_width;
	screen_ht = new_height;

	recalc_tile_extents();
	viewport_render();
}

void
viewport_render (void)
{
	viewport_gl_setup();

	layers_paint();
}

static void
viewport_gl_setup (void)
{
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho(0, screen_wd, 0, screen_ht, 0, 1);
	glViewport(0, 0, screen_wd, screen_ht);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	glTranslatef(0.375 - center_x + screen_wd / 2, 0.375 - center_y + screen_ht / 2, 0.0);

	glDisable(GL_BLEND);
	glDisable(GL_DEPTH_TEST);
	glDepthMask(GL_FALSE);

	shaders_init();
}

bool
viewport_within_world_bounds (void)
{
	unsigned int world_size = world_get_size();

	// Does part of the viewport show an area outside of the world?
	if (center_x < screen_wd / 2) return false;
	if (center_y < screen_ht / 2) return false;
	if (center_x + screen_wd / 2 >= world_size) return false;
	if (center_y + screen_ht / 2 >= world_size) return false;
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

unsigned int
viewport_get_center_x (void)
{
	return center_x;
}

unsigned int
viewport_get_center_y (void)
{
	return center_y;
}

int viewport_get_tile_top (void) { return tile_top; }
int viewport_get_tile_left (void) { return tile_left; }
int viewport_get_tile_right (void) { return tile_right; }
int viewport_get_tile_bottom (void) { return tile_bottom; }
