#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

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
	unsigned int size;
	unsigned int offset_x;
	unsigned int offset_y;
};

static void *xylist_deep_search (void *(*xylist_req)(struct xylist_req *), struct xylist_req *req, struct texture *tex);
static void draw_tile (int tile_x, int tile_y, GLuint texture_id, struct texture *t, double cx, double cy);
static GLuint texture_from_rawbits (void *rawbits);
static void *texture_request (struct xylist_req *req);
static void texture_destroy (void *data);

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
			req.xn = x;
			req.yn = y;
			req.zoom = world_zoom;
			req.xmin = tile_left;
			req.ymin = tile_top;
			req.xmax = tile_right;
			req.ymax = tile_bottom;
			req.search_depth = 9;

			// Is the texture already cached in OpenGL?
			if (textures && (txtdata = xylist_deep_search(texture_request, &req, &t)) != NULL) {
				// If the texture is already cached at native resolution,
				// then we're done; else still try to get the native bitmap:
				if (t.zoom == world_zoom) {
					draw_tile(x, y, (GLuint)txtdata, &t, cx, cy);
					continue;
				}
			}
			// Try to load the bitmap and turn it into an OpenGL texture:
			if ((bmpdata = xylist_deep_search(bitmap_request, &req, &t)) == NULL) {
				if (txtdata) {
					draw_tile(x, y, (GLuint)txtdata, &t, cx, cy);
				}
				continue;
			}
			GLuint id = texture_from_rawbits(bmpdata);
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
		return data;
	}
	// Didn't work, now try deeper zoom levels:
	for (int z = (int)req->zoom - 1; z >= 0; z--)
	{
		unsigned int zoomdiff = (req->zoom - z);
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

		// Don't go on a procure binge, just search current tiles in the xylist:
		zoomed_req.search_depth = 0;

		if ((data = xylist_req(&zoomed_req)) == NULL) {
			continue;
		}
		// At this zoom level, cut a block of this pixel size out of parent:
		t->size = (256 >> zoomdiff);
		t->zoom = z;

		// This is the nth block out of parent, counting from top left:
		xblock = (req->xn - (zoomed_req.xn << zoomdiff));
		yblock = (req->yn - (zoomed_req.yn << zoomdiff));

		// Reverse the yblock index, texture coordinates run from bottom left:
		yblock = (1 << zoomdiff) - 1 - yblock;

		t->offset_x = t->size * xblock;
		t->offset_y = t->size * yblock;
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
