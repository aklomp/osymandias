#include <stdbool.h>
#include <stdlib.h>
#include <GL/gl.h>

#include "../matrix.h"
#include "../inlinebin.h"
#include "../layers.h"
#include "../programs.h"
#include "../programs/cursor.h"
#include "../viewport.h"
#include "../png.h"

// Array of counterclockwise vertices:
//
//   3--2
//   |  |
//   0--1
//
static struct vertex {
	float x;
	float y;
	float u;
	float v;
} __attribute__((packed))
vertex[4] = {
	[0] = { .u = 0.0f, .v = 0.0f },
	[1] = { .u = 1.0f, .v = 0.0f },
	[2] = { .u = 1.0f, .v = 1.0f },
	[3] = { .u = 0.0f, .v = 1.0f },
};

// Array of indices. We define two counterclockwise triangles:
// 3-0-2 and 0-1-2
static GLubyte index[6] = {
	3, 0, 2,
	0, 1, 2,
};

// Projection matrix:
static struct {
	float proj[16];
} matrix;

// Screen size:
static struct {
	float width;
	float height;
} screen;

// Cursor texture:
static struct {
	char		*raw;
	const char	*png;
	size_t		 len;
	GLuint		 id;
	unsigned int	 width;
	unsigned int	 height;
} tex;

static GLuint vao, vbo;

static void
paint (void)
{
	// Viewport is screen:
	glViewport(0, 0, screen.width, screen.height);

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glDisable(GL_DEPTH_TEST);
	glDepthMask(GL_FALSE);

	// Use the cursor program:
	program_cursor_use(&((struct program_cursor) {
		.mat_proj = matrix.proj,
	}));

	// Activate cursor texture:
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, tex.id);

	// Draw all triangles in the buffer:
	glBindVertexArray(vao);
	glDrawElements(GL_TRIANGLES, sizeof(index) / sizeof(index[0]), GL_UNSIGNED_BYTE, index);

	program_none();
}

static void
resize (const unsigned int width, const unsigned int height)
{
	screen.width  = width;
	screen.height = height;

	// Projection matrix maps 1:1 to screen:
	mat_scale(matrix.proj, 2.0f / screen.width, 2.0f / screen.height, 0.0f);
}

static void
init_texture (void)
{
	// Generate texture:
	glGenTextures(1, &tex.id);
	glBindTexture(GL_TEXTURE_2D, tex.id);

	// Load cursor texture:
	inlinebin_get(TEXTURE_CURSOR, &tex.png, &tex.len);
	png_load(tex.png, tex.len, &tex.height, &tex.width, &tex.raw);

	// Upload texture:
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, tex.width, tex.height, 0, GL_RGBA, GL_UNSIGNED_BYTE, tex.raw);

	// Free memory:
	free(tex.raw);

	int halfwd = tex.width  / 2;
	int halfht = tex.height / 2;

	// Size vertex array to texture:
	vertex[0].x = -halfwd; vertex[0].y = -halfht;
	vertex[1].x =  halfwd; vertex[1].y = -halfht;
	vertex[2].x =  halfwd; vertex[2].y =  halfht;
	vertex[3].x = -halfwd; vertex[3].y =  halfht;

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
}

static bool
init (void)
{
	GLint loc;

	// Init texture:
	init_texture();

	// Generate vertex buffer object:
	glGenBuffers(1, &vbo);

	// Generate vertex array object:
	glGenVertexArrays(1, &vao);

	// Bind buffer and vertex array:
	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	glBindVertexArray(vao);

	// Add pointer to 'vertex' attribute:
	loc = program_cursor_loc(LOC_CURSOR_VERTEX);
	glEnableVertexAttribArray(loc);
	glVertexAttribPointer(loc, 2, GL_FLOAT, GL_FALSE,
		sizeof(struct vertex),
		(void *)(&((struct vertex *)0)->x));

	// Add pointer to 'texture' attribute:
	loc = program_cursor_loc(LOC_CURSOR_TEXTURE);
	glEnableVertexAttribArray(loc);
	glVertexAttribPointer(loc, 2, GL_FLOAT, GL_FALSE,
		sizeof(struct vertex),
		(void *)(&((struct vertex *)0)->u));

	// Copy vertices to buffer:
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertex), vertex, GL_STATIC_DRAW);

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

const struct layer *
layer_cursor (void)
{
	static struct layer layer = {
		.init     = &init,
		.paint    = &paint,
		.resize   = &resize,
		.destroy  = &destroy,
	};

	return &layer;
}
