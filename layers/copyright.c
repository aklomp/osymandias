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
	double ortho[16];
	double translate[16];
	double proj64[16];
	float  proj32[16];
} matrix;

// Screen size:
static struct {
	float width;
	float height;
} screen;

// Cursor texture:
static struct glutil_texture tex = {
	.src  = TEXTURE_COPYRIGHT,
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

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	// Use the cursor program:
	program_tile2d_use(&((struct program_tile2d) {
		.mat_proj = matrix.proj32,
	}));

	// Activate copyright texture:
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

	// Create an orthographic projection:
	mat_ortho(matrix.ortho, 0.0f, screen.width, screen.height, 0.0f, 0.0f, 1.0f);

	// Create a translation matrix:
	double tex_orig_x = screen.width  - tex.width  - 10.0;
	double tex_orig_y = screen.height - tex.height - 10.0;
	mat_translate(matrix.translate, tex_orig_x, tex_orig_y, 0.0);

	// Multiply:
	mat_multiply(matrix.proj64, matrix.ortho, matrix.translate);
	mat_to_float(matrix.proj32, matrix.proj64);
}

static bool
init_texture (void)
{
	// Load texture:
	if (glutil_texture_load(&tex) == false)
		return false;

	// Size vertex array to texture:
	vertex[0].x = 0;         vertex[0].y = tex.height;
	vertex[1].x = tex.width; vertex[1].y = tex.height;
	vertex[2].x = tex.width; vertex[2].y = 0;
	vertex[3].x = 0;         vertex[3].y = 0;

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
LAYER(40) = {
	.init    = &init,
	.paint   = &paint,
	.resize  = &resize,
	.destroy = &destroy,
};
