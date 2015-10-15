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
vertex[4];

// Array of indices. We define two counterclockwise triangles:
// 3-0-2 and 0-1-2
static GLubyte index[6] = {
	3, 0, 2,
	0, 1, 2,
};

// Projection and view matrices:
static float mat_proj[16];
static float mat_view[16];

// Screen size:
static float wd;
static float ht;

static GLuint vao, vbo;

static bool
occludes (void)
{
	return false;
}

static void
vertcoords (void)
{
	// Bottom left:
	vertex[0].x = -15;
	vertex[0].y = -15;

	// Bottom right:
	vertex[1].x =  15;
	vertex[1].y = -15;

	// Top right:
	vertex[2].x =  15;
	vertex[2].y =  15;

	// Top left:
	vertex[3].x = -15;
	vertex[3].y =  15;
}

static void
paint (void)
{
	// Viewport is screen:
	glViewport(0, 0, wd, ht);

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glDisable(GL_DEPTH_TEST);
	glDepthMask(GL_FALSE);

	// Use the cursor program:
	program_cursor_use(&((struct program_cursor) {
		.mat_proj = mat_proj,
		.mat_view = mat_view,
	}));

	// Draw all triangles in the buffer:
	glBindVertexArray(vao);
	glDrawElements(GL_TRIANGLES, sizeof(index) / sizeof(index[0]), GL_UNSIGNED_BYTE, index);

	program_none();
}

static void
resize (const unsigned int width, const unsigned int height)
{
	wd = width;
	ht = height;

	// Projection matrix maps 1:1 to screen:
	mat_ortho(mat_proj, 0, wd, 0, ht, 0, 1);

	// Modelview matrix translates vertices to center screen:
	mat_translate(mat_view, wd / 2.0f, ht / 2.0f, 0.0f);
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

	// Populate vertex coordinates:
	vertcoords();

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

struct layer *
layer_cursor (void)
{
	static struct layer layer = {
		.init     = &init,
		.occludes = &occludes,
		.paint    = &paint,
		.resize   = &resize,
		.destroy  = &destroy,
	};

	return &layer;
}
