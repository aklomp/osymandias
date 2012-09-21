#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <GL/gl.h>

#include "shaders.h"
#include "xylist.h"
#include "autoscroll.h"
#include "bitmap_mgr.h"
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
char *scratch = NULL;
char *missing_tile = NULL;

static void viewport_gl_setup (void);
static void viewport_draw_bkgd (void);
static void viewport_draw_cursor (void);
static bool viewport_need_bkgd (void);
static void viewport_draw_tiles (void);
static int tile_request (struct xylist_req *req, char **rawbits, unsigned int *offset_x, unsigned int *offset_y, unsigned int *zoomfactor);
static void missing_tile_init (void);

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

bool
viewport_init (void)
{
	if ((scratch = malloc(256 * 256 * 3)) == NULL) {
		return false;
	}
	if ((missing_tile = malloc(256 * 256 * 3)) == NULL) {
		free(scratch);
		return false;
	}
	zoom = 0;
	world_size = total_canvas_size(zoom);
	center_x = center_y = world_size / 2;
	recalc_tile_extents();
	missing_tile_init();
	return true;
}

void
viewport_destroy (void)
{
	free(missing_tile);
	free(scratch);
}

void
viewport_zoom_in (const int screen_x, const int screen_y)
{
	if (zoom != 18) {
		zoom++;
		center_x *= 2;
		center_y *= 2;
		world_size = total_canvas_size(zoom);
		bitmap_zoom_change(zoom);
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
		bitmap_zoom_change(zoom);
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
	return 0;
}

static void
viewport_draw_tiles (void)
{
	struct xylist_req req;
	char *rawbits;
	unsigned int offset_x;
	unsigned int offset_y;
	unsigned int zoomfactor;

	glEnable(GL_TEXTURE_2D);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glPixelZoom(1.0, -1.0);
	for (int x = tile_left; x <= tile_right; x++) {
		for (int y = tile_top; y <= tile_bottom; y++) {
			unsigned int tile_x = x * 256 + screen_wd / 2  - center_x;
			unsigned int tile_y = (y + 1) * 256 + screen_ht / 2 - center_y;
			glWindowPos2i(tile_x, tile_y);
			req.xn = x;
			req.yn = y;
			req.zoom = zoom;
			req.xmin = tile_left;
			req.ymin = tile_top;
			req.xmax = tile_right;
			req.ymax = tile_bottom;
			if (!tile_request(&req, &rawbits, &offset_x, &offset_y, &zoomfactor)) {
				/* Draw the "missing tile" */
				glDrawPixels(256, 256, GL_RGB, GL_UNSIGNED_BYTE, missing_tile);
			}
			else {
				if (zoomfactor == 0) {
					glDrawPixels(256, 256, GL_RGB, GL_UNSIGNED_BYTE, rawbits);
				}
				else {
					char *r = rawbits;
					char *b = scratch;
					for (int rows = 0; rows < (256 >> zoomfactor); rows++) {
						for (int repy = 0; repy < (1 << zoomfactor); repy++) {
							for (int cols = 0; cols < (256 >> zoomfactor); cols++) {
								r = rawbits + ((offset_y + rows) * 256 + offset_x + cols) * 3;
								for (int repx = 0; repx < (1 << zoomfactor); repx++) {
									*b++ = r[0];
									*b++ = r[1];
									*b++ = r[2];
								}
							}
						}
					}
					glDrawPixels(256, 256, GL_RGB, GL_UNSIGNED_BYTE, scratch);
				}
			}
		}
	}
	glDisable(GL_BLEND);
	glDisable(GL_TEXTURE_2D);
	glUseProgram(0);
}

static int
tile_request (struct xylist_req *req, char **rawbits, unsigned int *offset_x, unsigned int *offset_y, unsigned int *zoomfactor)
{
	void *data;

	req->search_depth = 9;

	// Is the texture already loaded?
	if ((data = bitmap_request(req)) != NULL) {
		*rawbits = data;
		*offset_x = 0;
		*offset_y = 0;
		*zoomfactor = 0;
		return 1;
	}
	// Otherwise, can we find the texture at lower zoomlevels?
	for (int z = (int)req->zoom - 1; z >= 0; z--)
	{
		unsigned int zoomdiff = (req->zoom - z);
		unsigned int blocksz;
		unsigned int xblock;
		unsigned int yblock;
		struct xylist_req zoomed_req;

		// Create our own request, based on the one we have, but zoomed:
		zoomed_req.xn = req->xn >> zoomdiff;
		zoomed_req.yn = req->yn >> zoomdiff;
		zoomed_req.zoom = (unsigned int)z;
		zoomed_req.xmin = req->xmin >> zoomdiff;
		zoomed_req.ymin = req->ymin >> zoomdiff;
		zoomed_req.xmax = req->xmax >> zoomdiff;
		zoomed_req.ymax = req->ymax >> zoomdiff;

		// Don't go to disk or to the network:
		zoomed_req.search_depth = 0;

		// Request the tile from the xylist with a search_depth of 1,
		// so that we will first try the texture cache, then try to procure
		// from the bitmap cache, and then give up:
		if ((data = bitmap_request(&zoomed_req)) == NULL) {
			continue;
		}
		// At this zoom level, cut a block of this pixel size out of parent:
		blocksz = (256 >> zoomdiff);
		// This is the nth block out of parent, counting from top left:
		xblock = (req->xn - (zoomed_req.xn << zoomdiff));
		yblock = (req->yn - (zoomed_req.yn << zoomdiff));

		// Reverse the yblock index, texture coordinates run from bottom left:
		yblock = (1 << zoomdiff) - 1 - yblock;

		*rawbits = data;
		*offset_x = blocksz * xblock;
		*offset_y = blocksz * yblock;
		*zoomfactor = zoomdiff;
		return 1;
	}
	return 0;
}

static void
missing_tile_init (void)
{
	// Background color:
	memset(missing_tile, 35, 256 * 256 * 3);

	// Border around bottom:
	for (char *p = missing_tile + 255 * 256 * 3; p < missing_tile + 256 * 256 * 3; p++) {
		*p = 51;
	}
	// Border around left:
	for (char *p = missing_tile; p < missing_tile + 256 * 256 * 3; p += 256 * 3) {
		p[0] = p[1] = p[2] = 51;
	}
}
