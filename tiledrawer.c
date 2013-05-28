#include <stdbool.h>
#include <GL/gl.h>

#include "quadtree.h"
#include "world.h"

struct texture {
	unsigned int tile_x;
	unsigned int tile_y;
	unsigned int wd;
	unsigned int ht;
	unsigned int offset_x;
	unsigned int offset_y;
	unsigned int zoomdiff;
};

static void
cutout_texture (int orig_x, int orig_y, int wd, int ht, const struct quadtree_req *req, struct texture *t)
{
	t->zoomdiff = req->world_zoom - req->found_zoom;
	t->tile_x = req->found_x;
	t->tile_y = req->found_y;

	// This is the nth block out of parent, counting from top left:
	int xblock = orig_x & ((1 << t->zoomdiff) - 1);
	int yblock = orig_y & ((1 << t->zoomdiff) - 1);

	if (t->zoomdiff >= 8) {
		t->offset_x = xblock >> (t->zoomdiff - 8);
		t->offset_y = yblock >> (t->zoomdiff - 8);
		t->wd = wd >> (t->zoomdiff - 8);
		t->ht = ht >> (t->zoomdiff - 8);
		return;
	}
	// Multiplication before division, avoid clipping:
	t->offset_x = (256 * xblock) >> t->zoomdiff;
	t->offset_y = (256 * yblock) >> t->zoomdiff;
	t->wd = (256 * wd) >> t->zoomdiff;
	t->ht = (256 * ht) >> t->zoomdiff;
}

static void
draw_tile_planar (int tile_x, int tile_y, int tile_wd, int tile_ht, GLuint texture_id, const struct texture *t, double cx, double cy)
{
	GLdouble txoffs = (GLdouble)t->offset_x / 256.0;
	GLdouble tyoffs = (GLdouble)t->offset_y / 256.0;
	GLdouble twd = (GLdouble)t->wd / 256.0;
	GLdouble tht = (GLdouble)t->ht / 256.0;

	glBindTexture(GL_TEXTURE_2D, texture_id);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

	// Flip tile y coordinates: tile origin is top left, world origin is bottom left:
	tile_y = world_get_size() - tile_y - tile_ht;

	// Need to calculate the world coordinates of our tile in double
	// precision, then translate the coordinates to the center ourselves.
	// OpenGL uses floats to represent the world, which lack the precision
	// to represent individual pixels at the max zoom level.
	float x[4] =
	{ -cx + (double)tile_x
	, -cx + (double)tile_x + (double)tile_wd
	, -cx + (double)tile_x + (double)tile_wd
	, -cx + (double)tile_x
	};
	float y[4] =
	{ -cy + (double)tile_y + (double)tile_ht
	, -cy + (double)tile_y + (double)tile_ht
	, -cy + (double)tile_y
	, -cy + (double)tile_y
	};
	glBegin(GL_QUADS);
		glTexCoord2f(txoffs,       tyoffs);       glVertex2f(x[0], y[0]);
		glTexCoord2f(txoffs + twd, tyoffs);       glVertex2f(x[1], y[1]);
		glTexCoord2f(txoffs + twd, tyoffs + tht); glVertex2f(x[2], y[2]);
		glTexCoord2f(txoffs,       tyoffs + tht); glVertex2f(x[3], y[3]);
	glEnd();
}

void
tiledrawer (int tile_x, int tile_y, int tile_wd, int tile_ht, double cx, double cy, GLuint texture_id, const struct quadtree_req *req)
{
	struct texture t;

	// If we couldn't find the texture at the requested resolution and
	// had to settle for a lower-res one, we need to cut out the proper
	// sub-block from the texture. Calculate coords of the block we need:
	cutout_texture(tile_x, tile_y, tile_wd, tile_ht, req, &t);

	// With this cutout information in hand, draw the actual tile:
	draw_tile_planar(tile_x, tile_y, tile_wd, tile_ht, texture_id, &t, cx, cy);
}
