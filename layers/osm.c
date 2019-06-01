#include <stdbool.h>
#include <string.h>
#include <math.h>

#include <GL/gl.h>

#include "../bitmap_cache.h"
#include "../texture_cache.h"
#include "../tiledrawer.h"
#include "../tilepicker.h"
#include "../layers.h"
#include "../inlinebin.h"
#include "../programs.h"

static bool
init (const struct viewport *vp)
{
	(void) vp;

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

static const struct cache_data *
find_texture (const struct cache_node *in, struct cache_node *out)
{
	const struct cache_data *cdata_tex, *cdata_bmp;
	struct cache_node out_tex, out_bmp;

	// Return true if the exact requested texture was found:
	if ((cdata_tex = texture_cache_search(in, &out_tex)) != NULL)
		if (in->zoom == out_tex.zoom) {
			*out = out_tex;
			return cdata_tex;
		}

	// Otherwise try to find a bitmap of higher zoom:
	bitmap_cache_lock();
	if ((cdata_bmp = bitmap_cache_search(in, &out_bmp)) != NULL) {
		if (cdata_tex == NULL || out_bmp.zoom > out_tex.zoom) {
			cdata_tex = texture_cache_insert(&out_bmp, cdata_bmp);
			bitmap_cache_unlock();
			*out = out_bmp;
			return cdata_tex;
		}
	}
	bitmap_cache_unlock();

	// Else use whatever texture we have:
	*out = out_tex;
	return cdata_tex;
}

static void
paint (const struct camera *cam, const struct viewport *vp)
{
	// Draw to world coordinates:
	viewport_gl_setup_world();

	// Load tiledrawer programs:
	tiledrawer_start(cam, vp);

	for (const struct tilepicker *tile = tilepicker_first(); tile; tile = tilepicker_next()) {

		// The tilepicker can tell us to draw a tile at a lower zoom
		// level than the world zoom; scale the tile's coordinates to
		// its native zoom level:
		struct cache_node out, in = {
			.x    = tile->x,
			.y    = tile->y,
			.zoom = tile->zoom,
		};

		tiledrawer(&(struct tiledrawer) {
			.tile = &out,
			.data = find_texture(&in, &out),
		});
	}

	program_none();
}

// Export public methods:
LAYER(20) = {
	.init    = &init,
	.paint   = &paint,
	.destroy = &destroy,
};
