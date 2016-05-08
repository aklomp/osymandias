#include <stdbool.h>

#include <GL/gl.h>

#include "../matrix.h"
#include "../vector.h"
#include "../worlds.h"
#include "../camera.h"
#include "../inlinebin.h"
#include "../glutil.h"
#include "../programs.h"
#include "../programs/cursor.h"
#include "../viewport.h"
#include "../layers.h"

#define MEMBERS(x)	(sizeof(x) / sizeof((x)[0]))

static struct {
	float scale[16];
	float translate[16];
	float project[16];
} matrix;

// Array of counterclockwise vertices:
//
//   3--2
//   |  |
//   0--1
//
static struct glutil_vertex_uv vertex_planar[4] = {
	[0] = {
		.x = -0.5f, .y = -0.5f,
		.u =  0.0f, .v =  1.0f
	},
	[1] = {
		.x = 0.5f, .y = -0.5f,
		.u = 1.0f, .v =  1.0f
	},
	[2] = {
		.x = 0.5f, .y = 0.5f,
		.u = 1.0f, .v = 0.0f
	},
	[3] = {
		.x = -0.5f, .y = 0.5f,
		.u =  0.0f, .v = 0.0f
	},
};

// Basemap texture:
static struct glutil_texture tex = {
	.src  = TEXTURE_BASEMAP,
	.type = GL_RGB,
};

// Vertex array and buffer objects:
static GLuint vao[1], vbo[1];
static const GLuint *vao_planar = &vao[0];
static const GLuint *vbo_planar = &vbo[0];

static void
zoom (const unsigned int zoom)
{
	(void)zoom;

	float world_size = world_get_size();

	// Scale 1x1 planar quad to new world size:
	mat_scale(matrix.scale, world_size, world_size, 1.0f);
}

static void
move_planar (void)
{
	const struct coords *center = world_get_center();
	float halfsz = world_get_size() / 2.0f;

	// Create translation matrix:
	// FIXME: this should eventually use world_get_matrix():
	mat_translate(matrix.translate, halfsz - center->tile.x, center->tile.y - halfsz, 0.0f);
}

static void
paint_planar (void)
{
	// Draw to world coordinates:
	viewport_gl_setup_world();

	// Create translation matrix:
	move_planar();

	// Multiply matrices:
	mat_multiply(matrix.project, matrix.translate, matrix.scale);
	mat_multiply(matrix.project, camera_mat_viewproj(), matrix.project);

	// Use the cursor program:
	program_cursor_use(&((struct program_cursor) {
		.mat_proj = matrix.project,
	}));

	// Activate basemap texture:
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, tex.id);

	// Draw all triangles in the buffer:
	glBindVertexArray(*vao_planar);
	glutil_draw_quad();

	program_none();
}

static void
paint_spherical (void)
{
}

static void
paint (void)
{
	switch (world_get())
	{
	case WORLD_PLANAR:
		paint_planar();
		break;

	case WORLD_SPHERICAL:
		paint_spherical();
		break;
	}
}

static void
init_planar (void)
{
	// Planar basemap: bind buffer and vertex array:
	glBindBuffer(GL_ARRAY_BUFFER, *vbo_planar);
	glBindVertexArray(*vao_planar);

	// Link 'vertex' and 'texture' attributes:
	glutil_vertex_uv_link(
		program_cursor_loc(LOC_CURSOR_VERTEX),
		program_cursor_loc(LOC_CURSOR_TEXTURE));

	// Copy vertices to buffer:
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertex_planar), vertex_planar, GL_STATIC_DRAW);
}

static bool
init (void)
{
	// Load basemap texture:
	if (glutil_texture_load(&tex) == false)
		return false;

	// Generate vertex buffer and array objects:
	glGenBuffers(MEMBERS(vbo), vbo);
	glGenVertexArrays(MEMBERS(vao), vao);

	init_planar();

	zoom(0);

	return true;
}

static void
destroy (void)
{
	// Delete texture:
	glDeleteTextures(1, &tex.id);

	// Delete vertex array and buffer objects:
	glDeleteVertexArrays(MEMBERS(vao), vao);
	glDeleteBuffers(MEMBERS(vbo), vbo);
}

const struct layer *
layer_basemap (void)
{
	static struct layer layer = {
		.init     = &init,
		.paint    = &paint,
		.zoom     = &zoom,
		.destroy  = &destroy,
	};

	return &layer;
}
