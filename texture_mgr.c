#include <stdbool.h>
#include <stdlib.h>
#include <GL/gl.h>

#include "bitmap_mgr.h"

#define N_ELEM(a)	(sizeof(a) / sizeof(a[0]))

struct texture {
	GLuint id;
	unsigned int zoom;
	unsigned int xn;
	unsigned int yn;
};

static struct texture *textures[100];
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

GLuint
texture_request (const unsigned int zoom, const unsigned int xn, const unsigned int yn)
{
	void *rawbits;

	// Is the texture already loaded?
	for (int i = 0; i < nextfree; i++) {
		if (textures[i]->id != 0 && textures[i]->zoom == zoom && textures[i]->xn == xn && textures[i]->yn == yn) {
			return textures[i]->id;
		}
	}
	// Out of space?
	if (nextfree == N_ELEM(textures)) {
		return 0;
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
		return textures[nextfree - 1]->id;
	}
	return 0;
}
