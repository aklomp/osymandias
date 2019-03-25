#include <math.h>

#include "camera.h"
#include "glutil.h"
#include "tiledrawer.h"
#include "programs.h"
#include "programs/planar.h"
#include "programs/spherical.h"
#include "worlds.h"

struct texture {
	float wd;
	float ht;
	struct {
		float x;
		float y;
	} offset;
};

void
tiledrawer (const struct tiledrawer *td)
{
	struct texture tex;
	const struct tilepicker *tile = td->pick;

	unsigned int zoomdiff = td->zoom.world - td->zoom.found;

	// If we couldn't find the texture at the requested resolution and
	// had to settle for a lower-res one, we need to cut out the proper
	// sub-block from the texture. Calculate coords of the block we need:

	// This is the nth block out of parent, counting from top left:
	float xblock = fmodf(tile->pos.x, (1 << zoomdiff));
	float yblock = fmodf(tile->pos.y, (1 << zoomdiff));

	// Multiplication before division, avoid clipping:
	tex.offset.x = ldexpf(xblock, 8 - zoomdiff);
	tex.offset.y = ldexpf(yblock, 8 - zoomdiff);
	tex.wd = ldexpf(tile->size.wd, 8 - zoomdiff);
	tex.ht = ldexpf(tile->size.ht, 8 - zoomdiff);

	float txoffs = ldexpf(tex.offset.x, -8);
	float tyoffs = ldexpf(tex.offset.y, -8);
	float twd = ldexpf(tex.wd, -8);
	float tht = ldexpf(tex.ht, -8);

	// Texture coordinates:
	struct glutil_vertex_uv texuv[4] = {
		{ .u = txoffs,       .v = tyoffs       },
		{ .u = txoffs + twd, .v = tyoffs       },
		{ .u = txoffs + twd, .v = tyoffs + tht },
		{ .u = txoffs,       .v = tyoffs + tht },
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
	texuv[0].x = texuv[3].x = tile->pos.x;
	texuv[1].x = texuv[2].x = tile->pos.x + tile->size.wd;
	texuv[0].y = texuv[1].y = tile->pos.y;
	texuv[3].y = texuv[2].y = tile->pos.y + tile->size.ht;

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
			.world_size   = world_get_size(),
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
			.world_zoom   = world_get_zoom(),
		}));
	}
}
