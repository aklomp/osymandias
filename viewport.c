#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>

#include <GL/gl.h>

#include "shaders.h"
#include "texture_mgr.h"
#include "viewport.h"

static unsigned int world_size = 0;	// current world size in pixels
static unsigned int zoom = 0;
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
static void viewport_draw_bkgd (void);
static void viewport_draw_cursor (void);
static bool viewport_need_bkgd (void);
static void viewport_draw_tiles (void);
static void glQuadTextured (const float, const float);

static unsigned int
total_canvas_size (const int zoom)
{
	// Zoom level 0 = 256 pixels
	// Zoom level 1 = 512 pixels
	// Zoom level 2 = 1024 pixels
	// Zoom level n = 2^(n + 8)

	return ((unsigned int)1) << (zoom + 8);
}

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
	tile_last = world_size / 256 - 1;
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

void
viewport_init (void)
{
	zoom = 0;
	world_size = total_canvas_size(zoom);
	center_x = center_y = world_size / 2;
	recalc_tile_extents();
}

void
viewport_destroy (void)
{
}

void
viewport_zoom_in (const int screen_x, const int screen_y)
{
	if (zoom != 18) {
		zoom++;
		center_x *= 2;
		center_y *= 2;
		world_size = total_canvas_size(zoom);
	}
	// Keep same point under mouse cursor:
	int dx = screen_x - screen_wd / 2;
	int dy = screen_y - screen_ht / 2;
	viewport_scroll(-dx, -dy);
}

void
viewport_zoom_out (const int screen_x, const int screen_y)
{
	if (zoom != 0) {
		zoom--;
		center_x /= 2;
		center_y /= 2;
		world_size = total_canvas_size(zoom);
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
	screen_wd = new_width;
	screen_ht = new_height;

	recalc_tile_extents();
	viewport_render();
}

void
viewport_render (void)
{
	viewport_gl_setup();
	if (viewport_need_bkgd()) {
		viewport_draw_bkgd();
	}
	viewport_draw_tiles();
	viewport_draw_cursor();
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

static void
viewport_draw_cursor (void)
{
	float halfwd = (float)screen_wd / 2.0;
	float halfht = (float)screen_ht / 2.0;

	shader_use_cursor(halfwd, halfht);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glBegin(GL_QUADS);
		glVertex2f((float)center_x - 15.0, (float)center_y - 15.0);
		glVertex2f((float)center_x + 15.0, (float)center_y - 15.0);
		glVertex2f((float)center_x + 15.0, (float)center_y + 15.0);
		glVertex2f((float)center_x - 15.0, (float)center_y + 15.0);
	glEnd();
	glUseProgram(0);
	glDisable(GL_BLEND);
}

static void
viewport_draw_bkgd (void)
{
	// Keep some margin to be on the sure side:
	float halfwd = (float)screen_wd / 2.0 + 1.0;
	float halfht = (float)screen_ht / 2.0 + 1.0;

	shader_use_bkgd(world_size, (center_x - screen_wd / 2) & 0xFF, (center_y - screen_ht / 2) & 0xFF);
	glBegin(GL_QUADS);
		glVertex2f(center_x - halfwd, center_y - halfht);
		glVertex2f(center_x + halfwd, center_y - halfht);
		glVertex2f(center_x + halfwd, center_y + halfht);
		glVertex2f(center_x - halfwd, center_y + halfht);
	glEnd();
	glUseProgram(0);
}

static bool
viewport_need_bkgd (void)
{
	// Out-of-bounds situation?
	if (tile_left <= 0 || tile_top <= 0 || tile_right >= tile_last || tile_bottom >= tile_last) {
		return true;
	}
	// If we are not sure that the area is covered, better draw the background:
	return !texture_area_is_covered(zoom, tile_left, tile_top, tile_right, tile_bottom);
}

static void
viewport_draw_tiles (void)
{
	struct texture *texture;
	unsigned int screen_offs_x = (center_x - screen_wd / 2) & 0xFF;
	unsigned int screen_offs_y = (center_y - screen_ht / 2) & 0xFF;

	glEnable(GL_TEXTURE_2D);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	for (int x = tile_left; x <= tile_right; x++) {
		for (int y = tile_top; y <= tile_bottom; y++) {
			if ((texture = texture_request(zoom, x, y)) == NULL) {
				continue;
			}
			shader_use_tile(screen_offs_x, screen_offs_y, texture->offset_x, texture->offset_y, texture->zoomfactor);
			glBindTexture(GL_TEXTURE_2D, texture->id);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
			glQuadTextured(x * 256, y * 256);
		}
	}
	glDisable(GL_BLEND);
	glDisable(GL_TEXTURE_2D);
	glUseProgram(0);
}

static void
glQuadTextured (const float x1, const float y1)
{
	// Add a 10px margin around each tile:
	// The float precision is not that great at 2^26 pixels
	float rx1 = x1 - 10.0;
	float rx2 = x1 + 266.0;
	float ry1 = y1 - 10.0;
	float ry2 = y1 + 266.0;

	// Draw the quad:
	glBegin(GL_QUADS);
		glTexCoord2f(0.0f, 0.0f); glVertex2f(rx1, ry1);
		glTexCoord2f(1.0f, 0.0f); glVertex2f(rx2, ry1);
		glTexCoord2f(1.0f, 1.0f); glVertex2f(rx2, ry2);
		glTexCoord2f(0.0f, 1.0f); glVertex2f(rx1, ry2);
	glEnd();
}
