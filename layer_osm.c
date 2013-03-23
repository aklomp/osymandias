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
#include "tilepicker.h"
#include "layers.h"
#include "layer_osm.h"

struct texture {
	int zoom;
	unsigned int tile_x;
	unsigned int tile_y;
	unsigned int wd;
	unsigned int ht;
	unsigned int offset_x;
	unsigned int offset_y;
	unsigned int zoomdiff;
};

static void *xylist_deep_search (void *(*xylist_req)(struct xylist_req *), struct xylist_req *req, struct texture *tex);
static void draw_tile (int tile_x, int tile_y, int tile_wd, int tile_ht, GLuint texture_id, struct texture *t, double cx, double cy);
static GLuint texture_from_rawbits (void *rawbits);
static void *texture_request (struct xylist_req *req);
static void texture_destroy (void *data);
static void zoomed_texture_cutout (int orig_x, int orig_y, int wd, int ht, struct texture *t);

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
	int x;
	int y;
	int tile_wd, tile_ht;
	int zoom;
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

	for (int iter = tilepicker_first(&x, &y, &tile_wd, &tile_ht, &zoom); iter; iter = tilepicker_next(&x, &y, &tile_wd, &tile_ht, &zoom))
	{
		// The tilepicker can tell us to draw a tile at a different zoom level to the world zoom;
		// we need to correct the geometry to reflect that:
		req.xn = x >> (world_zoom - zoom);
		req.yn = y >> (world_zoom - zoom);
		req.zoom = zoom;
		req.xmin = tile_left >> (world_zoom - zoom);
		req.ymin = tile_top >> (world_zoom - zoom);
		req.xmax = tile_right >> (world_zoom - zoom);
		req.ymax = tile_bottom >> (world_zoom - zoom);
		req.search_depth = 9;

		// Is the texture already cached in OpenGL?
		if (textures && (txtdata = xylist_deep_search(texture_request, &req, &t)) != NULL) {
			// If the texture is already cached at native resolution,
			// then we're done; else still try to get the native bitmap:
			if (t.zoom == zoom) {
				t.zoomdiff = world_zoom - t.zoom;
				zoomed_texture_cutout(x, y, tile_wd, tile_ht, &t);
				draw_tile(x, y, tile_wd, tile_ht, (GLuint)txtdata, &t, cx, cy);
				continue;
			}
		}
		// Try to load the bitmap and turn it into an OpenGL texture:
		if ((bmpdata = xylist_deep_search(bitmap_request, &req, &t)) == NULL) {
			if (txtdata) {
				t.zoomdiff = world_zoom - t.zoom;
				zoomed_texture_cutout(x, y, tile_wd, tile_ht, &t);
				draw_tile(x, y, tile_wd, tile_ht, (GLuint)txtdata, &t, cx, cy);
			}
			continue;
		}
		GLuint id = texture_from_rawbits(bmpdata);
		t.zoomdiff = world_zoom - t.zoom;
		zoomed_texture_cutout(x, y, tile_wd, tile_ht, &t);
		draw_tile(x, y, tile_wd, tile_ht, id, &t, cx, cy);

		// When we insert this texture id in the texture cache,
		// we do so under its native zoom level, not the zoom level
		// of the current viewport:
		req.zoom = t.zoom;
		req.xn = t.tile_x;
		req.yn = t.tile_y;

		// Store the identifier in the textures cache, else delete;
		// HACK: dirty int-to-pointer conversion:
		if (!textures || !xylist_insert_tile(textures, &req, (void *)id)) {
			glDeleteTextures(1, &id);
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
draw_tile (int tile_x, int tile_y, int tile_wd, int tile_ht, GLuint texture_id, struct texture *t, double cx, double cy)
{
	GLdouble txoffs = (GLdouble)t->offset_x / 256.0;
	GLdouble tyoffs = (GLdouble)t->offset_y / 256.0;
	GLdouble twd = (GLdouble)t->wd / 256.0;
	GLdouble tht = (GLdouble)t->ht / 256.0;

	glBindTexture(GL_TEXTURE_2D, texture_id);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

	// Flip tile y coordinates: tile origin is top left, world origin is bottom left:
	tile_y = world_get_size() - tile_y - tile_ht;

	// Need to calculate the world coordinates of our tile in double
	// precision, then translate the coordinates to the center ourselves.
	// OpenGL uses floats to represent the world, which lack the precision
	// to represent individual pixels at the max zoom level.

	glBegin(GL_QUADS);
		glTexCoord2f(txoffs,       tyoffs);       glVertex2f(cx + (double)tile_x,                   cy + (double)tile_y + (double)tile_ht);
		glTexCoord2f(txoffs + twd, tyoffs);       glVertex2f(cx + (double)tile_x + (double)tile_wd, cy + (double)tile_y + (double)tile_ht);
		glTexCoord2f(txoffs + twd, tyoffs + tht); glVertex2f(cx + (double)tile_x + (double)tile_wd, cy + (double)tile_y);
		glTexCoord2f(txoffs,       tyoffs + tht); glVertex2f(cx + (double)tile_x,                   cy + (double)tile_y);
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
zoomed_texture_cutout (int orig_x, int orig_y, int wd, int ht, struct texture *t)
{
	// This is the nth block out of parent, counting from top left:
	int xblock = orig_x & ((1 << t->zoomdiff) - 1);
	int yblock = orig_y & ((1 << t->zoomdiff) - 1);

	if (t->zoomdiff >= 8) {
		t->offset_x = xblock >> (t->zoomdiff - 8);
		t->offset_y = yblock >> (t->zoomdiff - 8);
		t->wd = wd >> (t->zoomdiff - 8);
		t->ht = ht >> (t->zoomdiff - 8);
		return;
	}
	// Multiplication before division, avoid clipping:
	t->offset_x = (256 * xblock) >> t->zoomdiff;
	t->offset_y = (256 * yblock) >> t->zoomdiff;
	t->wd = (256 * wd) >> t->zoomdiff;
	t->ht = (256 * ht) >> t->zoomdiff;
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
