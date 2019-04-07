#include <stdbool.h>
#include <string.h>
#include <math.h>

#include <GL/gl.h>

#include "../bitmap_cache.h"
#include "../worlds.h"
#include "../viewport.h"
#include "../texture_cache.h"
#include "../tiledrawer.h"
#include "../tilepicker.h"
#include "../layers.h"
#include "../inlinebin.h"
#include "../programs.h"

static bool
init (void)
{
	if (bitmap_cache_create())
		return texture_cache_create();

	bitmap_cache_destroy();
	return false;
}

static void
destroy (void)
{
	texture_cache_destroy();
	bitmap_cache_destroy();
}

static uint32_t
find_texture (const struct cache_node *in, struct cache_node *out)
{
	uint32_t id;
	void *rawbits;
	struct cache_node out_tex, out_bmp;

	// Return true if the exact requested texture was found:
	if ((id = texture_cache_search(in, &out_tex)) > 0)
		if (in->zoom == out_tex.zoom) {
			*out = out_tex;
			return id;
		}

	// Otherwise try to find a bitmap of higher zoom:
	bitmap_cache_lock();
	if ((rawbits = bitmap_cache_search(in, &out_bmp)) != NULL) {
		if (id == 0 || out_bmp.zoom > out_tex.zoom) {
			id = texture_cache_insert(&out_bmp, rawbits);
			bitmap_cache_unlock();
			*out = out_bmp;
			return id;
		}
	}
	bitmap_cache_unlock();

	// Else use whatever texture we have:
	*out = out_tex;
	return id;
}

static void
paint (void)
{
	int world_zoom = world_get_zoom();

	// Draw to world coordinates:
	viewport_gl_setup_world();

	// Load tiledrawer programs:
	tiledrawer_start();

	for (const struct tilepicker *tile = tilepicker_first(); tile; tile = tilepicker_next()) {

		// The tilepicker can tell us to draw a tile at a lower zoom
		// level than the world zoom; scale the tile's coordinates to
		// its native zoom level:
		struct cache_node out, in = {
			.x    = tile->x,
			.y    = tile->y,
			.zoom = tile->zoom,
		};
		uint32_t id;

		if ((id = find_texture(&in, &out)) == 0)
			continue;

		tiledrawer(&((struct tiledrawer) {
			.tile = &out,
			.world_zoom = world_zoom,
			.texture_id = id,
		}));
	}

	program_none();
}

// Export public methods:
LAYER(20) = {
	.init    = &init,
	.paint   = &paint,
	.destroy = &destroy,
};
