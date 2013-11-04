#include <stdbool.h>
#include <stdlib.h>
#include <stddef.h>
#include <string.h>
#include <math.h>

#include <GL/gl.h>

#include "quadtree.h"
#include "bitmap_mgr.h"
#include "shaders.h"
#include "world.h"
#include "viewport.h"
#include "tilepicker.h"
#include "tiledrawer.h"
#include "layers.h"
#include "layer_osm.h"

static GLuint texture_from_rawbits (void *rawbits);
static void texture_destroy (void *data);
static void set_zoom_color (int zoomlevel);

static int overlay_zoom = 0;
static int colorize_cache = 0;
static struct quadtree *textures = NULL;

static void
layer_osm_destroy (void *data)
{
	(void)data;

	quadtree_destroy(&textures);
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
	float x, y, tile_wd, tile_ht;
	int zoom;
	float p[4][3];
	struct quadtree_req req;
	struct quadtree_req req_tex;
	int world_zoom = world_get_zoom();

	// Draw to world coordinates:
	viewport_gl_setup_world();

	glEnable(GL_TEXTURE_2D);
	double cx = viewport_get_center_x();
	double cy = viewport_get_center_y();

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glDisable(GL_BLEND);

	// The texture colors are multiplied with this value:
	glColor3f(1.0, 1.0, 1.0);

	for (int iter = tilepicker_first(&x, &y, &tile_wd, &tile_ht, &zoom, p); iter; iter = tilepicker_next(&x, &y, &tile_wd, &tile_ht, &zoom, p))
	{
		// If showing the zoom colors overlay, pick proper mixin color:
		if (overlay_zoom) {
			set_zoom_color(zoom);
		}
		// The tilepicker can tell us to draw a tile at a different zoom level to the world zoom;
		// we need to correct the geometry to reflect that:
		req.x = ldexpf(x, -(world_zoom - zoom));
		req.y = ldexpf(y, -(world_zoom - zoom));
		req.zoom = zoom;
		req.world_zoom = world_zoom;
		req.cx = cx;
		req.cy = world_get_size() - cy;

		req_tex.found_data = NULL;

		// Is the texture already cached in OpenGL?
		if (quadtree_request(textures, &req)) {
			// If the texture is already cached at native resolution,
			// then we're done; else still try to get the native bitmap:
			if (req.found_zoom == zoom) {
				if (colorize_cache) glColor3f(0.3, 1.0, 0.3);
				tiledrawer(x, y, tile_wd, tile_ht, (GLuint)(ptrdiff_t)req.found_data, &req, p);
				if (colorize_cache) glColor3f(1.0, 1.0, 1.0);
				continue;
			}
			// Save found texture:
			memcpy(&req_tex, &req, sizeof(req));
		}
		// Try to load the bitmap and turn it into an OpenGL texture:
		if (bitmap_request(&req)) {
			// If we found a texture, and it's better or equal than
			// the bitmap we came back with, use that instead:
			if (req_tex.found_data != NULL && req_tex.found_zoom >= req.found_zoom) {
				if (colorize_cache) glColor3f(0.3, 1.0, 0.3);
				tiledrawer(x, y, tile_wd, tile_ht, (GLuint)(ptrdiff_t)req_tex.found_data, &req_tex, p);
				if (colorize_cache) glColor3f(1.0, 1.0, 1.0);
				continue;
			}
			GLuint id = texture_from_rawbits(req.found_data);
			if (colorize_cache) glColor3f(0.8, 0.0, 0.0);
			tiledrawer(x, y, tile_wd, tile_ht, id, &req, p);
			if (colorize_cache) glColor3f(1.0, 1.0, 1.0);

			req.zoom = req.found_zoom;
			req.x = req.found_x;
			req.y = req.found_y;

			if (!quadtree_data_insert(textures, &req, (void *)(ptrdiff_t)id)) {
				texture_destroy((void *)(ptrdiff_t)id);
			}
			continue;
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
texture_destroy (void *data)
{
	// HACK: dirty cast from pointer to int:
	GLuint id = (GLuint)(ptrdiff_t)data;

	glDeleteTextures(1, &id);
}

static void
set_zoom_color (int zoom)
{
	float zoomcolors[6][3] = {
		{ 1.0, 0.2, 0.2 },
		{ 0.2, 1.0, 0.2 },
		{ 0.2, 0.2, 1.0 },
		{ 0.7, 0.7, 0.2 },
		{ 0.2, 0.7, 0.7 },
		{ 0.7, 0.2, 0.7 }
	};
	glColor3f(zoomcolors[zoom % 6][0], zoomcolors[zoom % 6][1], zoomcolors[zoom % 6][2]);
}

bool
layer_osm_create (void)
{
	if (bitmap_mgr_init() == 0) {
		return 0;
	}
	// OpenGL texture cache:
	textures = quadtree_create(50, NULL, &texture_destroy);

	return layer_register(layer_create(layer_osm_full_occlusion, layer_osm_paint, layer_osm_zoom, layer_osm_destroy, NULL), 100);
}
