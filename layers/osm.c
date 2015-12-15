#include <stdbool.h>
#include <string.h>
#include <math.h>

#include <GL/gl.h>

#include "../quadtree.h"
#include "../bitmap_mgr.h"
#include "../worlds.h"
#include "../viewport.h"
#include "../vector.h"
#include "../tilepicker.h"
#include "../tiledrawer.h"
#include "../layers.h"
#include "../inlinebin.h"
#include "../programs.h"

static int overlay_zoom = 0;
static int colorize_cache = 0;
static struct quadtree *textures = NULL;

static bool
have_anisotropic (void)
{
	static bool init = false;
	static bool have = false;

	if (!init) {
		const char *ext = (const char *)glGetString(GL_EXTENSIONS);
		have = strstr(ext, "GL_EXT_texture_filter_anisotropic");
		init = true;
	}

	return have;
}

static GLuint
texture_from_rawbits (void *rawbits)
{
	GLuint id;

	glGenTextures(1, &id);
	glBindTexture(GL_TEXTURE_2D, id);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, 256, 256, 0, GL_RGB, GL_UNSIGNED_BYTE, rawbits);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

	// Apply anisotropic filtering if available:
	if (have_anisotropic()) {
		GLfloat max;
		glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &max);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, max);
	}

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

static bool
init (void)
{
	if (!bitmap_mgr_init())
		return false;

	// OpenGL texture cache:
	textures = quadtree_create(50, NULL, &texture_destroy);
	return true;
}

static void
destroy (void)
{
	quadtree_destroy(&textures);
	bitmap_mgr_destroy();
}

static bool
occludes (void)
{
	return false;
}

static void
tile_draw (const struct tilepicker *tile, const struct quadtree_req *req, GLuint id)
{
	if (colorize_cache)
		glColor3f(0.3, 1.0, 0.3);

	tiledrawer(&((struct tiledrawer) {
		.pick = tile,
		.zoom = {
			.world = req->world_zoom,
			.found = req->found_zoom,
		},
		.texture_id = id,
	}));

	if (colorize_cache)
		glColor3f(1.0, 1.0, 1.0);
}

// Check if the requested tile is already cached as a texture inside OpenGL.
// Return true if we drew the tile from the OpenGL cache, false if not.
static bool
texture_request (const struct tilepicker *tile, struct quadtree_req *req)
{
	// Return false if no matching texture was found:
	if (!quadtree_request(textures, req))
		return false;

	// Also return if the matching texture does not have native resolution;
	// we'll try to find a better fitting cached bitmap first:
	if (req->found_zoom != req->world_zoom)
		return false;

	// The texture has native resolution, draw it and return successfully:
	tile_draw(tile, req, (GLuint)(ptrdiff_t)req->found_data);
	return true;
}

static void
paint (void)
{
	struct tilepicker tile;
	int world_zoom = world_get_zoom();
	const struct coords *center = world_get_center();

	// Draw to world coordinates:
	viewport_gl_setup_world();

	glEnable(GL_TEXTURE_2D);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glDisable(GL_BLEND);

	// The texture colors are multiplied with this value:
	glColor3f(1.0, 1.0, 1.0);

	for (bool iter = tilepicker_first(&tile); iter; iter = tilepicker_next(&tile)) {

		// If showing the zoom colors overlay, pick proper mixin color:
		if (overlay_zoom)
			set_zoom_color(tile.zoom);

		// The tilepicker can tell us to draw a tile at a lower zoom
		// level than the world zoom; scale the tile's coordinates to
		// its native zoom level:
		struct quadtree_req req = {
			.x          = ldexpf(tile.pos.x, (int)tile.zoom - world_zoom),
			.y          = ldexpf(tile.pos.y, (int)tile.zoom - world_zoom),
			.zoom       = tile.zoom,
			.world_zoom = world_zoom,
			.center     = center,
			.found_data = NULL,
		};

		struct quadtree_req req_tex = req;	// OpenGL texture
		struct quadtree_req req_bmp = req;	// Cached bitmap

		// See if the exact matching texture is already cached in OpenGL:
		if (texture_request(&tile, &req_tex))
			continue;

		// Otherwise see if we can find a cached bitmap:
		bitmap_request(&req_bmp);

		// If we found a cached texture and no bitmap, or a bitmap of
		// lower zoom level, use the cached texture:
		if (req_tex.found_data)
			if (!req_bmp.found_data || req_tex.found_zoom >= req_bmp.found_zoom) {
				tile_draw(&tile, &req_tex, (GLuint)(ptrdiff_t)req_tex.found_data);
				continue;
			}

		// If we didn't find a bitmap, give up:
		if (!req_bmp.found_data)
			continue;

		// Else turn the bitmap into an OpenGL texture:
		GLuint id = texture_from_rawbits(req_bmp.found_data);

		// Draw it:
		tile_draw(&tile, &req_bmp, id);

		// Insert into the texture cache:
		req.zoom = req_bmp.found_zoom;
		req.x = req_bmp.found_x;
		req.y = req_bmp.found_y;

		if (!quadtree_data_insert(textures, &req, (void *)(ptrdiff_t)id))
			texture_destroy((void *)(ptrdiff_t)id);
	}

	glDisable(GL_BLEND);
	glDisable(GL_TEXTURE_2D);
	program_none();
}

static void
zoom (const unsigned int zoom)
{
	bitmap_zoom_change(zoom);
}

const struct layer *
layer_osm (void)
{
	static struct layer layer = {
		.init     = &init,
		.occludes = &occludes,
		.paint    = &paint,
		.zoom     = &zoom,
		.destroy  = &destroy,
	};

	return &layer;
}
