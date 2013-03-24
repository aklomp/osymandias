#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include <GL/gl.h>

#include "xylist.h"
#include "bitmap_mgr.h"
#include "shaders.h"
#include "world.h"
#include "viewport.h"
#include "layers.h"
#include "layer_osm.h"

struct texture {
	int zoom;
	unsigned int tile_x;
	unsigned int tile_y;
	unsigned int size;
	unsigned int offset_x;
	unsigned int offset_y;
	unsigned int zoomdiff;
};

static void *xylist_deep_search (void *(*xylist_req)(struct xylist_req *), struct xylist_req *req, struct texture *tex);
static void draw_tile (int tile_x, int tile_y, GLuint texture_id, struct texture *t, double cx, double cy);
static GLuint texture_from_rawbits (void *rawbits);
static void *texture_request (struct xylist_req *req);
static void texture_destroy (void *data);
static void zoomed_texture_cutout (int orig_x, int orig_y, struct texture *t);
static int tile_get_pixelwd (int tile_x, int tile_y);

static struct xylist *textures = NULL;

static void
layer_osm_destroy (void *data)
{
	(void)data;

	xylist_destroy(&textures);
	bitmap_mgr_destroy();
}

static bool
layer_osm_full_occlusion (void)
{
	return false;
}

static void
layer_osm_paint (void)
{
	void *txtdata = NULL;
	void *bmpdata = NULL;
	struct texture t;
	struct xylist_req req;
	int world_zoom = world_get_zoom();
	int tile_top = viewport_get_tile_top();
	int tile_left = viewport_get_tile_left();
	int tile_right = viewport_get_tile_right();
	int tile_bottom = viewport_get_tile_bottom();

	// Draw to world coordinates:
	viewport_gl_setup_world();

	glEnable(GL_TEXTURE_2D);
	double cx = -viewport_get_center_x();
	double cy = -viewport_get_center_y();

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glDisable(GL_BLEND);

	// The texture colors are multiplied with this value:
	glColor3f(1.0, 1.0, 1.0);

	for (int x = tile_left; x <= tile_right; x++) {
		for (int y = tile_top; y <= tile_bottom; y++) {

			// Calculate pixel width of tile on screen; this guides our
			// selection of zoom level (faraway tiles get less detail):
			int tile_pixelwd = tile_get_pixelwd(x, y);
			int tile_zoom = world_zoom;
			int zoomdiff;

			if (tile_pixelwd < 128) tile_zoom--;
			if (tile_pixelwd < 64) tile_zoom--;
			if (tile_pixelwd < 32) tile_zoom--;
			if (tile_pixelwd < 16) tile_zoom--;
			if (tile_pixelwd < 8) tile_zoom--;
			if (tile_pixelwd < 4) tile_zoom--;
			if (tile_pixelwd < 2) tile_zoom--;

			if (tile_zoom < 0) tile_zoom = 0;
			zoomdiff = world_zoom - tile_zoom;

			req.xn = x >> zoomdiff;
			req.yn = y >> zoomdiff;
			req.zoom = tile_zoom;
			req.xmin = tile_left >> zoomdiff;
			req.ymin = tile_top >> zoomdiff;
			req.xmax = tile_right >> zoomdiff;
			req.ymax = tile_bottom >> zoomdiff;
			req.search_depth = 9;

			// Is the texture already cached in OpenGL?
			if (textures && (txtdata = xylist_deep_search(texture_request, &req, &t)) != NULL) {
				// If the texture is already cached at native resolution,
				// then we're done; else still try to get the native bitmap:
				if (t.zoom == tile_zoom) {
					t.zoomdiff = world_zoom - t.zoom;
					zoomed_texture_cutout(x, y, &t);
					draw_tile(x, y, (GLuint)txtdata, &t, cx, cy);
					continue;
				}
			}
			// Try to load the bitmap and turn it into an OpenGL texture:
			if ((bmpdata = xylist_deep_search(bitmap_request, &req, &t)) == NULL) {
				if (txtdata) {
					t.zoomdiff = world_zoom - t.zoom;
					zoomed_texture_cutout(x, y, &t);
					draw_tile(x, y, (GLuint)txtdata, &t, cx, cy);
				}
				continue;
			}
			GLuint id = texture_from_rawbits(bmpdata);
			t.zoomdiff = world_zoom - t.zoom;
			zoomed_texture_cutout(x, y, &t);
			draw_tile(x, y, id, &t, cx, cy);

			// When we insert this texture id in the texture cache,
			// we do so under its native zoom level, not the zoom level
			// of the current viewport:
			req.zoom = t.zoom;

			// Store the identifier in the textures cache, else delete;
			// HACK: dirty int-to-pointer conversion:
			if (!textures || !xylist_insert_tile(textures, &req, (void *)id)) {
				glDeleteTextures(1, &id);
			}
		}
	}
	glDisable(GL_BLEND);
	glDisable(GL_TEXTURE_2D);
	glUseProgram(0);
}

static void
layer_osm_zoom (void)
{
	bitmap_zoom_change(world_get_zoom());
}

static void *
xylist_deep_search (void *(*xylist_req)(struct xylist_req *), struct xylist_req *req, struct texture *t)
{
	void *data;

	// Traverse all zoom levels of the given xylist from current
	// to lowest, looking for a tile that matches the request.

	// First, try the callback at the current zoom level:
	if ((data = xylist_req(req)) != NULL) {
		t->offset_x = 0;
		t->offset_y = 0;
		t->size = 256;
		t->zoom = req->zoom;
		t->zoomdiff = 0;
		t->tile_x = req->xn;
		t->tile_y = req->yn;
		return data;
	}
	// Didn't work, now try deeper zoom levels:
	for (int z = (int)req->zoom - 1; z >= 0; z--)
	{
		unsigned int zoomdiff = (req->zoom - z);
		struct xylist_req zoomed_req;

		// Create our own request, based on the one we have, but zoomed:
		zoomed_req.xn = req->xn >> zoomdiff;
		zoomed_req.yn = req->yn >> zoomdiff;
		zoomed_req.zoom = (unsigned int)z;
		zoomed_req.xmin = req->xmin >> zoomdiff;
		zoomed_req.ymin = req->ymin >> zoomdiff;
		zoomed_req.xmax = req->xmax >> zoomdiff;
		zoomed_req.ymax = req->ymax >> zoomdiff;

		// Don't go on a procure binge, just search current tiles in the xylist:
		zoomed_req.search_depth = 0;

		if ((data = xylist_req(&zoomed_req)) == NULL) {
			continue;
		}
		t->zoom = z;
		t->zoomdiff = zoomdiff;
		t->tile_x = zoomed_req.xn;
		t->tile_y = zoomed_req.yn;

		return data;
	}
	return NULL;
}

static GLuint
texture_from_rawbits (void *rawbits)
{
	GLuint id;

	glGenTextures(1, &id);
	glBindTexture(GL_TEXTURE_2D, id);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, 256, 256, 0, GL_RGB, GL_UNSIGNED_BYTE, rawbits);

	return id;
}

static void
draw_tile (int tile_x, int tile_y, GLuint texture_id, struct texture *t, double cx, double cy)
{
	GLdouble txoffs = (GLdouble)t->offset_x / 256.0;
	GLdouble tyoffs = (GLdouble)t->offset_y / 256.0;
	GLdouble tsize = (GLdouble)t->size / 256.0;

	glBindTexture(GL_TEXTURE_2D, texture_id);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

	// Flip tile y coordinates: tile origin is top left, world origin is bottom left:
	tile_y = world_get_size() - 1 - tile_y;

	// Need to calculate the world coordinates of our tile in double
	// precision, then translate the coordinates to the center ourselves.
	// OpenGL uses floats to represent the world, which lack the precision
	// to represent individual pixels at the max zoom level.

	glBegin(GL_QUADS);
		glTexCoord2f(txoffs,         tyoffs);         glVertex2f(cx + (double)tile_x,       cy + (double)tile_y + 1.0);
		glTexCoord2f(txoffs + tsize, tyoffs);         glVertex2f(cx + (double)tile_x + 1.0, cy + (double)tile_y + 1.0);
		glTexCoord2f(txoffs + tsize, tyoffs + tsize); glVertex2f(cx + (double)tile_x + 1.0, cy + (double)tile_y);
		glTexCoord2f(txoffs,         tyoffs + tsize); glVertex2f(cx + (double)tile_x,       cy + (double)tile_y);
	glEnd();
}

static void *
texture_request (struct xylist_req *req)
{
	// Helper function to provide the function signature needed by
	// xylist_deep_search.

	return xylist_request(textures, req);
}

static void
texture_destroy (void *data)
{
	// HACK: dirty cast from pointer to int:
	GLuint id = (GLuint)data;

	glDeleteTextures(1, &id);
}

static void
zoomed_texture_cutout (int orig_x, int orig_y, struct texture *t)
{
	// At this zoom level, cut a block of this pixel size out of parent:
	t->size = 256 >> t->zoomdiff;

	// This is the nth block out of parent, counting from top left:
	int xblock = orig_x & ((1 << t->zoomdiff) - 1);
	int yblock = orig_y & ((1 << t->zoomdiff) - 1);

	t->offset_x = t->size * xblock;
	t->offset_y = t->size * yblock;
}

static int
tile_get_pixelwd (int tile_x, int tile_y)
{
	// For the given tile, calculate the width on screen of its closest edge.
	// Useful to determine which zoom level to use for the tile.

	// Corner points of the tile, clockwise from top left:
	double px[4] = { tile_x, tile_x + 1, tile_x + 1, tile_x };
	double py[4] = { tile_y + 1, tile_y + 1, tile_y, tile_y };
	double tx[2], ty[2];

	// Get rotation in world:
	float rot = viewport_get_rot();

	// Get coordinates of longest visible rib:
	if (rot > 45.0 && rot <= 135.0) {
		tx[0] = px[0]; ty[0] = py[0];
		tx[1] = px[3]; ty[1] = py[3];
	}
	else if (rot > 135.0 && rot <= 225.0) {
		tx[0] = px[0]; ty[0] = py[0];
		tx[1] = px[1]; ty[1] = py[1];
	}
	else if (rot > 225.0 && rot <= 315.0) {
		tx[0] = px[1]; ty[0] = py[1];
		tx[1] = px[2]; ty[1] = py[2];
	}
	else {
		tx[0] = px[3]; ty[0] = py[3];
		tx[1] = px[2]; ty[1] = py[2];
	}
	// Project these points on the screen:
	double sx[2], sy[2];

	viewport_world_to_screen(tx[0], ty[0], &sx[0], &sy[0]);
	viewport_world_to_screen(tx[1], ty[1], &sx[1], &sy[1]);

	// Calculate rib length:
	double dx = sx[0] - sx[1];
	double dy = sy[0] - sy[1];

	return (int)sqrt(dx * dx + dy * dy);
}

bool
layer_osm_create (void)
{
	if (bitmap_mgr_init() == 0) {
		return 0;
	}
	// OpenGL texture cache, NULL is also acceptable:
	textures = xylist_create(0, 18, 50, NULL, &texture_destroy);

	return layer_register(layer_create(layer_osm_full_occlusion, layer_osm_paint, layer_osm_zoom, layer_osm_destroy, NULL), 100);
}
