#include <GL/gl.h>

#include "tiledrawer.h"
#include "programs.h"
#include "program/spherical.h"

// Array of indices. If we have a quad defined by these corners:
//
//   3--2
//   |  |
//   0--1
//
// we define two counterclockwise triangles, 0-2-3 and 0-1-2:
static const uint8_t vertex_index[] = {
	0, 2, 3,
	0, 1, 2,
};

static struct {
	uint32_t vao;
} state;

void
tiledrawer (const struct tiledrawer *td)
{
	if (td->tex == NULL)
		return;

	// Set tile zoom level:
	program_spherical_set_tile(td->tile, &td->tex->coords);

	// Bind texture:
	glBindTexture(GL_TEXTURE_2D, td->tex->id);

	// Draw two triangles which together form one quad:
	glDrawElements(GL_TRIANGLES, sizeof (vertex_index), GL_UNSIGNED_BYTE, vertex_index);
}

void
tiledrawer_start (const struct camera *cam, const struct viewport *vp)
{
	static bool init = false;

	// Lazy init:
	if (init == false) {
		glGenVertexArrays(1, &state.vao);
		init = true;
	}

	glBindVertexArray(state.vao);
	program_spherical_use(cam, vp);

	glActiveTexture(GL_TEXTURE0);
	glEnable(GL_TEXTURE_2D);
}
