#include <stdbool.h>
#include <stdlib.h>
#include <GL/gl.h>

#include "../matrix.h"
#include "../inlinebin.h"
#include "../glutil.h"
#include "../layers.h"
#include "../programs.h"
#include "../programs/tile2d.h"
#include "../png.h"

// Array of counterclockwise vertices:
//
//   3--2
//   |  |
//   0--1
//
static struct glutil_vertex_uv vertex[4] = GLUTIL_VERTEX_UV_DEFAULT;

// Projection matrix:
static struct {
	float  proj32[16];
	double proj64[16];
} matrix;

// Screen size:
static struct {
	float width;
	float height;
} screen;

// Cursor texture:
static struct glutil_texture tex = {
	.src  = TEXTURE_CURSOR,
	.type = GL_RGBA,
};

static GLuint vao, vbo;

static void
paint (const struct camera *cam, const struct viewport *vp)
{
	(void) cam;
	(void) vp;

	// Viewport is screen:
	glViewport(0, 0, screen.width, screen.height);

	glDisable(GL_DEPTH_TEST);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	// Use the cursor program:
	program_tile2d_use(&((struct program_tile2d) {
		.mat_proj = matrix.proj32,
	}));

	// Activate cursor texture:
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, tex.id);

	// Draw all triangles in the buffer:
	glBindVertexArray(vao);
	glutil_draw_quad();

	program_none();
}

static void
resize (const struct viewport *vp)
{
	screen.width  = vp->width;
	screen.height = vp->height;

	// Projection matrix maps 1:1 to screen:
	mat_scale(matrix.proj64, 2.0 / screen.width, 2.0 / screen.height, 0.0);
	mat_to_float(matrix.proj32, matrix.proj64);
}

static bool
init_texture (void)
{
	// Load texture:
	if (glutil_texture_load(&tex) == false)
		return false;

	int halfwd = tex.width  / 2;
	int halfht = tex.height / 2;

	// Size vertex array to texture:
	vertex[0].x = -halfwd; vertex[0].y = -halfht;
	vertex[1].x =  halfwd; vertex[1].y = -halfht;
	vertex[2].x =  halfwd; vertex[2].y =  halfht;
	vertex[3].x = -halfwd; vertex[3].y =  halfht;

	return true;
}

static bool
init (const struct viewport *vp)
{
	// Init texture:
	if (init_texture() == false)
		return false;

	// Generate vertex buffer object:
	glGenBuffers(1, &vbo);

	// Generate vertex array object:
	glGenVertexArrays(1, &vao);

	// Bind buffer and vertex array:
	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	glBindVertexArray(vao);

	// Link 'vertex' and 'texture' attributes:
	glutil_vertex_uv_link(
		program_tile2d_loc(LOC_TILE2D_VERTEX),
		program_tile2d_loc(LOC_TILE2D_TEXTURE));

	// Copy vertices to buffer:
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertex), vertex, GL_STATIC_DRAW);

	resize(vp);
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
LAYER(60) = {
	.init    = &init,
	.paint   = &paint,
	.resize  = &resize,
	.destroy = &destroy,
};
