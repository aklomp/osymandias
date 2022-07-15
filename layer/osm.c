#include <stdbool.h>
#include <string.h>
#include <math.h>

#include <GL/gl.h>

#include "../bitmap_cache.h"
#include "../texture_cache.h"
#include "../tiledrawer.h"
#include "../tilepicker.h"
#include "../layer.h"
#include "../inlinebin.h"
#include "../program.h"

static bool
on_init (const struct viewport *vp)
{
	(void) vp;

	if (bitmap_cache_create())
		return texture_cache_create();

	bitmap_cache_destroy();
	return false;
}

static void
on_destroy (void)
{
	texture_cache_destroy();
	bitmap_cache_destroy();
}

static const struct texture_cache *
find_texture (const struct cache_node *in, struct cache_node *out)
{
	const struct bitmap_cache  *bitmap;
	const struct texture_cache *tex;
	struct cache_node out_bitmap, out_tex;

	// Return true if the exact requested texture was found:
	if ((tex = texture_cache_search(in, &out_tex)) != NULL)
		if (in->zoom == out_tex.zoom) {
			*out = out_tex;
			return tex;
		}

	// Otherwise try to find a bitmap of higher zoom:
	bitmap_cache_lock();
	if ((bitmap = bitmap_cache_search(in, &out_bitmap)) != NULL) {
		if (tex == NULL || out_bitmap.zoom > out_tex.zoom) {
			tex = texture_cache_insert(&out_bitmap, bitmap);
			bitmap_cache_unlock();
			*out = out_bitmap;
			return tex;
		}
	}
	bitmap_cache_unlock();

	// Else use whatever texture we have:
	*out = out_tex;
	return tex;
}

static void
on_paint (const struct camera *cam, const struct viewport *vp)
{
	glDisable(GL_BLEND);

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
			.tex  = find_texture(&in, &out),
		});
	}

	program_none();
}

static struct layer layer = {
	.name       = "Openstreetmap",
	.zdepth     = 20,
	.visible    = true,
	.on_init    = &on_init,
	.on_paint   = &on_paint,
	.on_destroy = &on_destroy,
};

LAYER_REGISTER(&layer)
