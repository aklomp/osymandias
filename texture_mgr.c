#include <stdbool.h>
#include <stdlib.h>
#include <GL/gl.h>

#include "xylist.h"
#include "bitmap_mgr.h"
#include "texture_mgr.h"

#define N_ELEM(a)	(sizeof(a) / sizeof(a[0]))

struct texdata {
	GLuint id;
};

static void *texture_procure (const unsigned int zoom, const unsigned int xn, const unsigned int yn, const unsigned int search_depth);
static void texture_destroy (void *data);

struct xylist *textures = NULL;
static struct texture output;		// Reuse this structure shamelessly

int
texture_mgr_init (void)
{
	return ((textures = xylist_create(0, 18, 100, &texture_procure, &texture_destroy)) == NULL) ? 0 : 1;
}

void
texture_mgr_destroy (void)
{
	xylist_destroy(&textures);
}

struct texture *
texture_request (const unsigned int zoom, const unsigned int xn, const unsigned int yn)
{
	void *data;

	// Is the texture already loaded?
	if ((data = xylist_request(textures, zoom, xn, yn, 9)) != NULL) {
		output.id = ((struct texdata *)data)->id;
		output.offset_x = 0;
		output.offset_y = 0;
		output.zoomfactor = 1;
		return &output;
	}
	// Otherwise, can we find the texture at lower zoomlevels?
	for (int z = (int)zoom - 1; z >= 0; z--)
	{
		unsigned int zoomdiff = (zoom - z);
		unsigned int zoomed_xn = xn >> (zoomdiff);
		unsigned int zoomed_yn = yn >> (zoomdiff);
		unsigned int blocksz;
		unsigned int xblock;
		unsigned int yblock;

		// Request the tile from the xylist with a search_depth of 1,
		// so that we will first try the texture cache, then try to procure
		// from the bitmap cache, and then give up:
		if ((data = xylist_request(textures, (unsigned int)z, zoomed_xn, zoomed_yn, 1)) == NULL) {
			continue;
		}
		// At this zoom level, cut a block of this pixel size out of parent:
		blocksz = (256 >> zoomdiff);
		// This is the nth block out of parent, counting from top left:
		xblock = (xn - (zoomed_xn << zoomdiff));
		yblock = (yn - (zoomed_yn << zoomdiff));

		// Reverse the yblock index, texture coordinates run from bottom left:
		yblock = (1 << zoomdiff) - 1 - yblock;

		output.id = ((struct texdata *)data)->id;
		output.offset_x = blocksz * xblock;
		output.offset_y = blocksz * yblock;
		output.zoomfactor = zoomdiff + 1;
		return &output;
	}
	return NULL;
}

static void *
texture_procure (const unsigned int zoom, const unsigned int xn, const unsigned int yn, const unsigned int search_depth)
{
	struct texdata *data;
	void *rawbits;

	// This function is called by the xylist code when it wants to fill a tile slot.
	// The return pointer is stored for us and given back when we search for a tile.
	// We create a texture id object by asking the bitmap manager for a bitmap.

	if ((data = malloc(sizeof(*data))) == NULL) {
		return NULL;
	}
	// Can we find rawbits to generate the texture?
	if ((rawbits = bitmap_request(zoom, xn, yn, search_depth)) == NULL) {
		free(data);
		return NULL;
	}
	glGenTextures(1, &data->id);
	glBindTexture(GL_TEXTURE_2D, data->id);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, 256, 256, 0, GL_RGB, GL_UNSIGNED_BYTE, rawbits);
	return data;
}

static void
texture_destroy (void *data)
{
	// The xylist code calls this when it wants to delete a tile:
	glDeleteTextures(1, &((struct texdata *)data)->id);
	free(data);
}

int
texture_area_is_covered (const unsigned int zoom, const unsigned int tile_xmin, const unsigned int tile_ymin, const unsigned int tile_xmax, const unsigned int tile_ymax)
{
	return xylist_area_is_covered(textures, zoom, tile_xmin, tile_ymin, tile_xmax, tile_ymax);
}
