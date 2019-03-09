#include <stdbool.h>
#include <stdlib.h>

#include <GL/gl.h>

#include "../png.h"
#include "../layers.h"
#include "../inlinebin.h"
#include "../glutil.h"
#include "../programs.h"
#include "../programs/bkgd.h"

// Array of counterclockwise vertices:
//
//   3--2
//   |  |
//   0--1
//
static struct glutil_vertex_uv vertex[4] =
{
	[0] = { .x = -1.0, .y = -1.0 },
	[1] = { .x =  1.0, .y = -1.0 },
	[2] = { .x =  1.0, .y =  1.0 },
	[3] = { .x = -1.0, .y =  1.0 },
};

// Background texture:
static struct glutil_texture tex = {
	.src  = TEXTURE_BACKGROUND,
	.type = GL_RGB,
};

static GLuint vao, vbo;

static void
texcoords (float screen_wd, float screen_ht)
{
	float wd = screen_wd / tex.width;
	float ht = screen_ht / tex.height;

	// Bottom left:
	vertex[0].u = 0;
	vertex[0].v = ht;

	// Bottom right:
	vertex[1].u = wd;
	vertex[1].v = ht;

	// Top right:
	vertex[2].u = wd;
	vertex[2].v = 0;

	// Top left:
	vertex[3].u = 0;
	vertex[3].v = 0;
}

static void
paint (void)
{
	// Use the background program:
	program_bkgd_use();

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, tex.id);

	// Copy vertices to buffer:
	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertex), vertex, GL_STATIC_DRAW);

	// Draw all triangles in the buffer:
	glBindVertexArray(vao);
	glutil_draw_quad();

	program_none();
}

static void
resize (const unsigned int width, const unsigned int height)
{
	// Update texture coordinates:
	texcoords(width, height);
}

static bool
init (void)
{
	// Load texture:
	if (glutil_texture_load(&tex) == false)
		return false;

	// Make a tiling texture:
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

	// Generate vertex buffer object:
	glGenBuffers(1, &vbo);

	// Generate vertex array object:
	glGenVertexArrays(1, &vao);

	// Bind buffer and vertex array:
	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	glBindVertexArray(vao);

	// Link 'vertex' and 'texture' attributes:
	glutil_vertex_uv_link(
		program_bkgd_loc(LOC_BKGD_VERTEX),
		program_bkgd_loc(LOC_BKGD_TEXTURE));

	return true;
}

static void
destroy (void)
{
	// Delete texture:
	glDeleteTextures(1, &tex.id);

	// Delete vertex array object:
	glDeleteVertexArrays(1, &vao);

	// Delete vertex buffer:
	glDeleteBuffers(1, &vbo);
}

// Export public methods:
LAYER(10) = {
	.init    = &init,
	.paint   = &paint,
	.resize  = &resize,
	.destroy = &destroy,
};
