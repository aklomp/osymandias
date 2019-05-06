#include "glutil.h"
#include "tiledrawer.h"
#include "programs.h"
#include "programs/spherical.h"

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

// Per-vertex data sent to OpenGL.
struct vertex {
	struct vertex_texture {
		float u;
		float v;
	} __attribute__((packed)) texture;
	struct cache_data_coords coords;
} __attribute__((packed));

static struct {
	uint32_t vao;
	uint32_t vbo;
} state;

void
tiledrawer (const struct tiledrawer *td)
{
	if (td->data == NULL)
		return;

	// Texture coordinates:
	struct vertex vertex[] = {
		{ .texture.u = 0.0f, .texture.v = 1.0f, .coords = td->data->coords.sw },
		{ .texture.u = 1.0f, .texture.v = 1.0f, .coords = td->data->coords.se },
		{ .texture.u = 1.0f, .texture.v = 0.0f, .coords = td->data->coords.ne },
		{ .texture.u = 0.0f, .texture.v = 0.0f, .coords = td->data->coords.nw },
	};

	// Set tile zoom level:
	program_spherical_set_tile(td->tile);

	glActiveTexture(GL_TEXTURE0);
	glEnable(GL_TEXTURE_2D);

	// Bind texture:
	glBindTexture(GL_TEXTURE_2D, td->data->u32);

	// Copy vertices to buffer:
	glBindBuffer(GL_ARRAY_BUFFER, state.vbo);
	glBufferData(GL_ARRAY_BUFFER, sizeof (vertex), vertex, GL_DYNAMIC_DRAW);

	// Draw two triangles which together form one quad:
	glDrawElements(GL_TRIANGLES, sizeof (vertex_index), GL_UNSIGNED_BYTE, vertex_index);
}

void
tiledrawer_init (void)
{
	const int32_t loc_texture = program_spherical_loc(LOC_SPHERICAL_TEXTURE);
	const int32_t loc_vertex  = program_spherical_loc(LOC_SPHERICAL_VERTEX);

	glGenVertexArrays(1, &state.vao);
	glGenBuffers(1, &state.vbo);
	glBindVertexArray(state.vao);
	glBindBuffer(GL_ARRAY_BUFFER, state.vbo);

	// Add pointer to texture coordinates:
	glEnableVertexAttribArray(loc_texture);
	glVertexAttribPointer(loc_texture, 2, GL_FLOAT, GL_FALSE,
			sizeof (struct vertex),
			(void *) offsetof(struct vertex, texture));

	// Add pointer to 3D vertex coordinates:
	glEnableVertexAttribArray(loc_vertex);
	glVertexAttribPointer(loc_vertex, 3, GL_FLOAT, GL_FALSE,
			sizeof (struct vertex),
			(void *) offsetof(struct vertex, coords));
}

void
tiledrawer_start (const struct camera *cam, const struct globe *globe)
{
	static bool init = false;

	// Lazy init:
	if (init == false) {
		tiledrawer_init();
		init = true;
	}

	glBindVertexArray(state.vao);
	glBindBuffer(GL_ARRAY_BUFFER, state.vbo);

	program_spherical_use(&((struct program_spherical) {
		.mat_viewproj = cam->matrix.viewproj,
		.mat_model    = globe->matrix.model,
	}));
}
