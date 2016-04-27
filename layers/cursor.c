#include <stdbool.h>
#include <GL/gl.h>

#include "../matrix.h"
#include "../inlinebin.h"
#include "../layers.h"
#include "../programs.h"
#include "../programs/cursor.h"
#include "../viewport.h"

// Array of counterclockwise vertices:
//
//   3--2
//   |  |
//   0--1
//
static struct vertex {
	float x;
	float y;
} __attribute__((packed))
vertex[4] =
{
	[0] = { -15, -15 },
	[1] = {  15, -15 },
	[2] = {  15,  15 },
	[3] = { -15,  15 },
};

// Array of indices. We define two counterclockwise triangles:
// 3-0-2 and 0-1-2
static GLubyte index[6] = {
	3, 0, 2,
	0, 1, 2,
};

// Projection and view matrices:
static struct {
	float proj[16];
	float view[16];
} matrix;

// Screen size:
static struct {
	float width;
	float height;
} screen;

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
		.mat_view = matrix.view,
	}));

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
	mat_ortho(matrix.proj, 0, screen.width, 0, screen.height, 0, 1);

	// Modelview matrix translates vertices to center screen:
	mat_translate(matrix.view, screen.width / 2.0f, screen.height / 2.0f, 0.0f);
}

static bool
init (void)
{
	// Generate vertex buffer object:
	glGenBuffers(1, &vbo);

	// Generate vertex array object:
	glGenVertexArrays(1, &vao);

	// Bind buffer and vertex array:
	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	glBindVertexArray(vao);

	// Add pointer to 'vertex' attribute:
	glEnableVertexAttribArray(program_cursor_loc_vertex());
	glVertexAttribPointer(program_cursor_loc_vertex(), 2, GL_FLOAT, GL_FALSE,
		sizeof(struct vertex),
		(void *)(&((struct vertex *)0)->x));

	// Copy vertices to buffer:
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertex), vertex, GL_STATIC_DRAW);

	return true;
}

static void
destroy (void)
{
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
