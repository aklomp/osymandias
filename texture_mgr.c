#include <stdbool.h>
#include <stdlib.h>
#include <GL/gl.h>

#include "bitmap_mgr.h"
#include "texture_mgr.h"

#define N_ELEM(a)	(sizeof(a) / sizeof(a[0]))

struct texdata {
	GLuint id;
	unsigned int zoom;
	unsigned int xn;
	unsigned int yn;
};

static struct texture *texture_find_zoomed (const unsigned int zoom, const unsigned int xn, const unsigned int yn);

static struct texdata *textures[100];
static struct texture output;		// Reuse this structure shamelessly
static int nextfree = 0;

int
texture_mgr_init (void)
{
	// FIXME for prototyping only:
	for (unsigned int i = 0; i < N_ELEM(textures); i++) {
		textures[i] = malloc(sizeof(struct texture));
		textures[i]->id = 0;
	}
	return 1;
}

void
texture_mgr_destroy (void)
{
	for (unsigned int i = 0; i < N_ELEM(textures); i++) {
		if (textures[i]->id > 0) {
			glDeleteTextures(1, &textures[i]->id);
		}
		free(textures[i]);
	}
}

struct texture *
texture_request (const unsigned int zoom, const unsigned int xn, const unsigned int yn)
{
	void *rawbits;

	// Is the texture already loaded?
	for (int i = 0; i < nextfree; i++) {
		if (textures[i]->id != 0 && textures[i]->zoom == zoom && textures[i]->xn == xn && textures[i]->yn == yn) {
			output.id = textures[i]->id;
			output.offset_x = 0;
			output.offset_y = 0;
			output.zoomfactor = 1;
			return &output;
		}
	}
	// Out of space?
	if (nextfree == N_ELEM(textures)) {
		return texture_find_zoomed(zoom, xn, yn);
	}
	// Can we find rawbits to generate the texture?
	if ((rawbits = bitmap_request(zoom, xn, yn)) != NULL) {
		glGenTextures(1, &textures[nextfree]->id);
		glBindTexture(GL_TEXTURE_2D, textures[nextfree]->id);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, 256, 256, 0, GL_RGB, GL_UNSIGNED_BYTE, rawbits);
		textures[nextfree]->zoom = zoom;
		textures[nextfree]->xn = xn;
		textures[nextfree]->yn = yn;
		nextfree++;
		output.id = textures[nextfree - 1]->id;
		output.offset_x = 0;
		output.offset_y = 0;
		output.zoomfactor = 1;
		return &output;
	}
	return texture_find_zoomed(zoom, xn, yn);
}

int
texture_quick_check (const unsigned int zoom, const unsigned int xn, const unsigned int yn)
{
	// TODO: this should be a real standalone routine:
	return (texture_request(zoom, xn, yn) != NULL);
}

static struct texture *
texture_find_zoomed (const unsigned int zoom, const unsigned int xn, const unsigned int yn)
{
	// Can we reuse a texture from a lower zoom level?
	for (int i = 0; i < nextfree; i++) {
		unsigned int zoomdiff;
		if (textures[i]->id == 0 || textures[i]->zoom >= zoom) {
			continue;
		}
		zoomdiff = zoom - textures[i]->zoom;
		if (textures[i]->xn == (xn >> zoomdiff) && textures[i]->yn == (yn >> zoomdiff)) {
			// At this zoom level, cut a block of this pixel size out of parent:
			unsigned int blocksz = (256 >> zoomdiff);
			// This is the nth block out of parent, counting from top left:
			unsigned int xblock = (xn - (textures[i]->xn << zoomdiff));
			unsigned int yblock = (yn - (textures[i]->yn << zoomdiff));

			// Reverse the yblock index, texture coordinates run from bottom left:
			yblock = (1 << zoomdiff) - 1 - yblock;

			output.id = textures[i]->id;
			output.offset_x = blocksz * xblock;
			output.offset_y = blocksz * yblock;
			output.zoomfactor = zoomdiff + 1;
			return &output;
		}
	}
	return NULL;
}
