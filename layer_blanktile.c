#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include <GL/gl.h>

#include "world.h"
#include "viewport.h"
#include "layers.h"

// Tile size is one pixel more than the real tile size, so that we can use a
// single tile with full borders around all edges, and overlap the tiles by 1px
// with their neighbors:
#define TSIZE  257

static char *tile = NULL;

static void
layer_blanktile_destroy (void *data)
{
	(void)data;

	free(tile);
}

static bool
layer_blanktile_full_occlusion (void)
{
	// If the viewport is within world bounds,
	// we will always fully occlude:
	return (viewport_within_world_bounds());
}

static void
layer_blanktile_paint (void)
{
	int tile_top = viewport_get_tile_top();
	int tile_left = viewport_get_tile_left();
	int tile_right = viewport_get_tile_right();
	int tile_bottom = viewport_get_tile_bottom();

	glEnable(GL_TEXTURE_2D);
	glDisable(GL_BLEND);
	for (int x = tile_left; x <= tile_right; x++) {
		for (int y = tile_top; y <= tile_bottom; y++) {
			unsigned int tile_x = x * 256 + viewport_get_wd() / 2 - viewport_get_center_x();
			unsigned int tile_y = (y + 1) * 256 + viewport_get_ht() / 2 - viewport_get_center_y();

			// Shift the tile inward by 1 pixel if not bottom or leftmost row;
			// the 1-pixel border of the tile will overlap with the border of its
			// left and bottom neighbors; note that the coordinate system is flipped,
			// which doesn't concern us:
			if (x != tile_left) {
				tile_x -= 1;
			}
			if (y != tile_top) {
				tile_y -= 1;
			}
			glWindowPos2i(tile_x, tile_y);
			glDrawPixels(TSIZE, TSIZE, GL_RGB, GL_UNSIGNED_BYTE, tile);
		}
	}
	glDisable(GL_TEXTURE_2D);
}

static void
tile_init (void)
{
	char *p;

	// Align strides to dword boundary with a ghetto modulo operation:
	int stride = TSIZE * 3 + (TSIZE - 4 * (TSIZE / 4));

	// Background color:
	memset(tile, 35, TSIZE * stride);

	// Border around top:
	for (p = tile; p < tile + stride; p += 3) {
		p[0] = p[1] = p[2] = 51;
	}
	// Border around bottom:
	for (p = tile + (TSIZE - 1) * stride; p < tile + TSIZE * stride; p += 3) {
		p[0] = p[1] = p[2] = 51;
	}
	// Border around left:
	for (p = tile; p < tile + (TSIZE - 1) * stride; p += stride) {
		p[0] = p[1] = p[2] = 51;
	}
	// Border around right:
	for (p = tile + (TSIZE - 1) * 3; p < tile + TSIZE * stride; p += stride) {
		p[0] = p[1] = p[2] = 51;
	}
}

bool
layer_blanktile_create (void)
{
	if ((tile = malloc(TSIZE * TSIZE * 3)) == NULL) {
		return false;
	}
	tile_init();
	return layer_register(layer_create(layer_blanktile_full_occlusion, layer_blanktile_paint, NULL, layer_blanktile_destroy, NULL), 10);
}
