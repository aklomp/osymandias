#include <math.h>

#include "camera.h"
#include "glutil.h"
#include "tiledrawer.h"
#include "programs.h"
#include "programs/planar.h"
#include "programs/spherical.h"
#include "worlds.h"

void
tiledrawer (const struct tiledrawer *td)
{
	const struct cache_node *tile = td->tile;

	// Texture coordinates:
	struct glutil_vertex_uv texuv[4] = {
		{ .u = 0.0f, 0.0f },
		{ .u = 1.0f, 0.0f },
		{ .u = 1.0f, 1.0f },
		{ .u = 0.0f, 1.0f },
	};

	// Vertex coordinates, translating tile coordinate order to OpenGL
	// coordinate order:
	//
	//  OpenGL   Tile
	//
	//   3--2    0,0----wd,0
	//   |  |     |      |
	//   0--1    0,ht--wd,ht
	//
	texuv[0].x = texuv[3].x = tile->x;
	texuv[1].x = texuv[2].x = tile->x + 1;
	texuv[0].y = texuv[1].y = tile->y;
	texuv[3].y = texuv[2].y = tile->y + 1;

	// Set tile zoom level:
	if (world_get() == WORLD_PLANAR)
		program_planar_set_tile_zoom(tile->zoom);
	else
		program_spherical_set_tile(tile);

	glActiveTexture(GL_TEXTURE0);
	glEnable(GL_TEXTURE_2D);

	// Bind texture:
	glBindTexture(GL_TEXTURE_2D, td->texture_id);

	// Copy vertices to buffer:
	glBufferData(GL_ARRAY_BUFFER, sizeof (texuv), texuv, GL_STATIC_DRAW);

	// Draw all triangles in the buffer:
	glutil_draw_quad();
}

void
tiledrawer_start (void)
{
	static GLuint vao;
	static bool init = false;

	// Lazy init:
	if (init == false) {
		glGenVertexArrays(1, &vao);
		init = true;
	}

	glBindVertexArray(vao);

	if (world_get() == WORLD_PLANAR) {
		glutil_vertex_uv_link(
			program_planar_loc(LOC_PLANAR_VERTEX),
			program_planar_loc(LOC_PLANAR_TEXTURE));

		program_planar_use(&((struct program_planar) {
			.mat_viewproj = camera_mat_viewproj(),
			.mat_model    = world_get_matrix(),
			.tile_zoom    = 0,
			.world_zoom   = world_get_zoom(),
		}));
	}
	else {
		glutil_vertex_uv_link(
			program_spherical_loc(LOC_SPHERICAL_VERTEX),
			program_spherical_loc(LOC_SPHERICAL_TEXTURE));

		program_spherical_loc(LOC_SPHERICAL_VERTEX),
		program_spherical_loc(LOC_SPHERICAL_TEXTURE);

		program_spherical_use(&((struct program_spherical) {
			.mat_viewproj = camera_mat_viewproj(),
			.mat_model    = world_get_matrix(),
			.tile_zoom    = 0,
		}));
	}
}
