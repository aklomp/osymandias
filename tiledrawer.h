#pragma once

#include <GL/gl.h>

struct tiledrawer {
	const struct tilepicker *pick;
	struct {
		unsigned int world;
		unsigned int found;
	} zoom;
	GLuint texture_id;
};

extern void tiledrawer (const struct tiledrawer *);
