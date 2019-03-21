#include <stdbool.h>

#include <GL/gl.h>

#include "../matrix.h"
#include "../worlds.h"
#include "../camera.h"
#include "../inlinebin.h"
#include "../glutil.h"
#include "../programs.h"
#include "../programs/tile2d.h"
#include "../viewport.h"
#include "../camera.h"
#include "../layers.h"
#include "../inlinebin.h"
#include "../png.h"
#include "../programs.h"
#include "../programs/basemap_spherical.h"
#include "../util.h"

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

static const struct glutil_vertex vertex_spherical[4] =
{
	[0] = { -1.0, -1.0 },
	[1] = {  1.0, -1.0 },
	[2] = {  1.0,  1.0 },
	[3] = { -1.0,  1.0 },
};

// Basemap texture:
static struct glutil_texture tex = {
	.src  = TEXTURE_BASEMAP,
	.type = GL_RGB,
};

// Vertex array and buffer objects:
static GLuint vao[2], vbo[2];
static const GLuint *vao_planar    = &vao[0];
static const GLuint *vao_spherical = &vao[1];
static const GLuint *vbo_planar    = &vbo[0];
static const GLuint *vbo_spherical = &vbo[1];

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
	program_tile2d_use(&((struct program_tile2d) {
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
	// Use the basemap_spherical program:
	program_basemap_spherical_use(&((struct program_basemap_spherical) {
		.mat_viewproj_inv = camera_mat_viewproj_inv(),
		.mat_model_inv = world_get_matrix_inverse(),
	}));

	// Draw all triangles in the buffer:
	glBindVertexArray(*vao_spherical);
	glutil_draw_quad();

	program_none();
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
		program_tile2d_loc(LOC_TILE2D_VERTEX),
		program_tile2d_loc(LOC_TILE2D_TEXTURE));

	// Copy vertices to buffer:
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertex_planar), vertex_planar, GL_STATIC_DRAW);
}

static void
init_spherical (void)
{
	// Spherical basemap: bind buffer and vertex array:
	glBindBuffer(GL_ARRAY_BUFFER, *vbo_spherical);
	glBindVertexArray(*vao_spherical);

	// Link 'vertex' attribute:
	glutil_vertex_link(program_basemap_spherical_loc_vertex());

	// Copy vertices to buffer:
	glBindBuffer(GL_ARRAY_BUFFER, *vbo_spherical);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertex_spherical), vertex_spherical, GL_STATIC_DRAW);
}

static bool
init (void)
{
	// Load basemap texture:
	if (glutil_texture_load(&tex) == false)
		return false;

	// Generate vertex buffer and array objects:
	glGenBuffers(NELEM(vbo), vbo);
	glGenVertexArrays(NELEM(vao), vao);

	init_planar();
	init_spherical();

	zoom(0);

	return true;
}

static void
destroy (void)
{
	// Delete texture:
	glDeleteTextures(1, &tex.id);

	// Delete vertex array and buffer objects:
	glDeleteVertexArrays(NELEM(vao), vao);
	glDeleteBuffers(NELEM(vbo), vbo);
}

// Export public methods:
LAYER(30) = {
	.init    = &init,
	.paint   = &paint,
	.zoom    = &zoom,
	.destroy = &destroy,
};
