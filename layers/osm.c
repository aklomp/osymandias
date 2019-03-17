#include <stdbool.h>
#include <string.h>
#include <math.h>

#include <GL/gl.h>

#include "../quadtree.h"
#include "../bitmap_mgr.h"
#include "../worlds.h"
#include "../viewport.h"
#include "../texture_cache.h"
#include "../tilepicker.h"
#include "../tiledrawer.h"
#include "../layers.h"
#include "../inlinebin.h"
#include "../programs.h"

// Cache sources for the drawn tile:
enum cache_source {
	CACHED_TEXTURE,
	CACHED_BITMAP,
};

// Local state:
static struct {
	bool overlay_zoom;
	bool cache_source_show;
}
state;

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
	if (bitmap_mgr_init())
		return texture_cache_create();

	return false;
}

static void
destroy (void)
{
	texture_cache_destroy();
	bitmap_mgr_destroy();
}

static void
tile_draw (const struct tilepicker *tile, const struct cache_node *req, uint32_t id, enum cache_source source)
{
	// Overlay a color mask to highlight texture's source:
	if (state.cache_source_show)
		switch (source) {
		case CACHED_TEXTURE:
			glColor3f(0.3, 1.0, 0.3);
			break;

		case CACHED_BITMAP:
			glColor3f(0.3, 0.3, 1.0);
			break;
		};

	// Draw tile:
	tiledrawer(&((struct tiledrawer) {
		.pick = tile,
		.zoom = {
			.world = world_get_zoom(),
			.found = req->zoom,
		},
		.texture_id = id,
	}));

	// Undo color mask:
	if (state.cache_source_show)
		glColor3f(1.0, 1.0, 1.0);
}

// Check if the requested tile is already cached as a texture inside OpenGL.
// Return true if we drew the tile from the OpenGL cache, false if not.
static bool
texture_request (const struct tilepicker *tile, const struct cache_node *in, struct cache_node *out, uint32_t *id)
{
	// Return false if no matching texture was found:
	if ((*id = texture_cache_search(in, out)) == 0)
		return false;

	// Also return if the matching texture does not have native resolution;
	// we'll try to find a better fitting cached bitmap first:
	if (out->zoom != in->zoom)
		return false;

	// The texture has native resolution, draw it and return successfully:
	tile_draw(tile, out, *id, CACHED_TEXTURE);
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

	// The texture colors are multiplied with this value:
	glColor3f(1.0, 1.0, 1.0);

	// Load tiledrawer programs:
	tiledrawer_start();

	for (bool iter = tilepicker_first(&tile); iter; iter = tilepicker_next(&tile)) {

		// If showing the zoom colors overlay, pick proper mixin color:
		if (state.overlay_zoom)
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

		struct quadtree_req req_bmp = req;	// Cached bitmap

		uint32_t id_tex;
		struct cache_node out_tex, in_tex = {
			.x    = req.x,
			.y    = req.y,
			.zoom = req.zoom,
		};

		// See if the exact matching texture is already cached in OpenGL:
		if (texture_request(&tile, &in_tex, &out_tex, &id_tex))
			continue;

		// Otherwise see if we can find a cached bitmap:
		bitmap_request(&req_bmp);

		// If we found a cached texture and no bitmap, or a bitmap of
		// lower zoom level, use the cached texture:
		if (id_tex > 0)
			if (!req_bmp.found_data || out_tex.zoom >= (uint32_t) req_bmp.found_zoom) {
				tile_draw(&tile, &out_tex, id_tex, CACHED_TEXTURE);
				continue;
			}

		// If we didn't find a bitmap, give up:
		if (!req_bmp.found_data)
			continue;

		// Else turn the bitmap into an OpenGL texture:
		const struct cache_node out_bmp = {
			.x    = req_bmp.found_x,
			.y    = req_bmp.found_y,
			.zoom = req_bmp.found_zoom,
		};

		uint32_t id = texture_cache_insert(&out_bmp, req_bmp.found_data);

		// Draw it:
		tile_draw(&tile, &out_bmp, id, CACHED_BITMAP);
	}

	program_none();
}

static void
zoom (const unsigned int zoom)
{
	bitmap_zoom_change(zoom);
}

// Export public methods:
LAYER(30) = {
	.init    = &init,
	.paint   = &paint,
	.zoom    = &zoom,
	.destroy = &destroy,
};
