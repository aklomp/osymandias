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

// Use textured quads to draw the tiles instead of using a direct drawing technique:
#define USE_QUADS

static bool tile_request (struct xylist_req *req, char **rawbits, unsigned int *offset_x, unsigned int *offset_y, unsigned int *zoomfactor);

#ifdef USE_QUADS
static void draw_textured_tile (int tile_x, int tile_y, void *rawbits, int xoffs, int yoffs, int size, double cx, double cy);
#else
static char *scratch = NULL;
#endif

static void
layer_osm_destroy (void *data)
{
	(void)data;

	bitmap_mgr_destroy();
#ifndef USE_QUADS
	free(scratch);
#endif
}

static bool
layer_osm_full_occlusion (void)
{
	return false;
}

static void
layer_osm_paint (void)
{
	struct xylist_req req;
	char *rawbits;
	unsigned int offset_x;
	unsigned int offset_y;
	unsigned int zoomfactor;
	int tile_top = viewport_get_tile_top();
	int tile_left = viewport_get_tile_left();
	int tile_right = viewport_get_tile_right();
	int tile_bottom = viewport_get_tile_bottom();

	// Draw to world coordinates:
	viewport_gl_setup_world();

	glEnable(GL_TEXTURE_2D);
#ifdef USE_QUADS
	double cx = -viewport_get_center_x();
	double cy = -viewport_get_center_y();

	glDisable(GL_BLEND);

	// The texture colors are multiplied with this value:
	glColor3f(1.0, 1.0, 1.0);
#else
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glPixelZoom(1.0, -1.0);
#endif
	for (int x = tile_left; x <= tile_right; x++) {
		for (int y = tile_top; y <= tile_bottom; y++) {
#ifndef USE_QUADS
			int tile_x, tile_y;

			viewport_world_to_screen(x, y + 1, &tile_x, &tile_y);
			glWindowPos2i(tile_x, tile_y);
#endif
			req.xn = x;
			req.yn = y;
			req.zoom = world_get_zoom();
			req.xmin = tile_left;
			req.ymin = tile_top;
			req.xmax = tile_right;
			req.ymax = tile_bottom;

			// Failed to find tile? Continue:
			if (!tile_request(&req, &rawbits, &offset_x, &offset_y, &zoomfactor)) {
				continue;
			}
			// Found tile at current zoomlevel? Blit:
			if (zoomfactor == 0) {
#ifdef USE_QUADS
				draw_textured_tile(x, y, rawbits, 0, 0, 256, cx, cy);
#else
				glDrawPixels(256, 256, GL_RGB, GL_UNSIGNED_BYTE, rawbits);
#endif
				continue;
			}
			// Different (lower) zoomlevel? Cut out the relevant
			// piece of the tile and enlarge it:
#ifdef USE_QUADS
			draw_textured_tile(x, y, rawbits, offset_x, offset_y, 256 >> zoomfactor, cx, cy);
#else
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
#endif
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

static bool
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
		return true;
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
		return true;
	}
	return false;
}

#ifdef USE_QUADS
static void
draw_textured_tile (int tile_x, int tile_y, void *rawbits, int xoffs, int yoffs, int size, double cx, double cy)
{
	GLuint id;
	GLdouble txoffs = (GLdouble)xoffs / 256.0;
	GLdouble tyoffs = (GLdouble)yoffs / 256.0;
	GLdouble tsize = (GLdouble)size / 256.0;

	glGenTextures(1, &id);
	glBindTexture(GL_TEXTURE_2D, id);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, 256, 256, 0, GL_RGB, GL_UNSIGNED_BYTE, rawbits);
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
	glDeleteTextures(1, &id);
}
#endif

bool
layer_osm_create (void)
{
#ifndef USE_QUADS
	if ((scratch = malloc(256 * 256 * 3)) == NULL) {
		return false;
	}
#endif
	bitmap_mgr_init();
	return layer_register(layer_create(layer_osm_full_occlusion, layer_osm_paint, layer_osm_zoom, layer_osm_destroy, NULL), 100);
}
