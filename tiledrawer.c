#include <stdbool.h>
#include <GL/gl.h>

#include "quadtree.h"
#include "coord3d.h"
#include "world.h"
#include "viewport.h"

struct texture {
	unsigned int tile_x;
	unsigned int tile_y;
	unsigned int wd;
	unsigned int ht;
	unsigned int offset_x;
	unsigned int offset_y;
	unsigned int zoomdiff;
};

#define MIN_SPHERE_DIVISIONS	16

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

static void
draw_tile_spherical (int tile_x, int tile_y, int tile_wd, int tile_ht, GLuint texture_id, const struct texture *t)
{
	float x[4], y[4], z[4];
	unsigned int world_size = world_get_size();
	unsigned int world_zoom = world_get_zoom();

	glBindTexture(GL_TEXTURE_2D, texture_id);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

	// If tile larger than world_size/MIN_SPHERE_DIVISIONS, split into multiple sub-tiles;
	// ensure the globe's diameter is always drawn with at least MIN_SPHERE_DIVISIONS segments:
	int xdivide = ((tile_wd * MIN_SPHERE_DIVISIONS) >> world_zoom); if (xdivide == 0) xdivide = 1;
	int ydivide = ((tile_ht * MIN_SPHERE_DIVISIONS) >> world_zoom); if (ydivide == 0) ydivide = 1;

	GLdouble twd = (GLdouble)t->wd / (256 * xdivide);
	GLdouble tht = (GLdouble)t->ht / (256 * ydivide);

	glBegin(GL_QUADS);
	for (int dx = 0; dx < xdivide; dx++) {
		for (int dy = 0; dy < ydivide; dy++) {
			GLdouble txoffs = (GLdouble)t->offset_x / 256.0 + dx * twd;
			GLdouble tyoffs = (GLdouble)t->offset_y / 256.0 + dy * tht;

			float lon1 = world_x_to_lon(tile_x + (float)(dx * tile_wd) / xdivide, world_size);
			float lon2 = world_x_to_lon(tile_x + (float)((dx + 1) * tile_wd) / xdivide, world_size);

			float lat1 = world_y_to_lat(tile_y + (float)(dy * tile_ht) / ydivide, world_size);
			float lat2 = world_y_to_lat(tile_y + (float)((dy + 1) * tile_ht) / ydivide, world_size);

			latlon_to_xyz(lat1, lon1, world_size, &x[0], &y[0], &z[0]);
			latlon_to_xyz(lat1, lon2, world_size, &x[1], &y[1], &z[1]);
			latlon_to_xyz(lat2, lon2, world_size, &x[2], &y[2], &z[2]);
			latlon_to_xyz(lat2, lon1, world_size, &x[3], &y[3], &z[3]);

			glNormal3f(x[0], y[0], z[0]); glTexCoord2f(txoffs,       tyoffs);       glVertex3f(x[0], y[0], z[0]);
			glNormal3f(x[1], y[1], z[1]); glTexCoord2f(txoffs + twd, tyoffs);       glVertex3f(x[1], y[1], z[1]);
			glNormal3f(x[2], y[2], z[2]); glTexCoord2f(txoffs + twd, tyoffs + tht); glVertex3f(x[2], y[2], z[2]);
			glNormal3f(x[3], y[3], z[3]); glTexCoord2f(txoffs,       tyoffs + tht); glVertex3f(x[3], y[3], z[3]);
		}
	}
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
	if (viewport_mode_get() == VIEWPORT_MODE_PLANAR) {
		draw_tile_planar(tile_x, tile_y, tile_wd, tile_ht, texture_id, &t, cx, cy);
	}
	if (viewport_mode_get() == VIEWPORT_MODE_SPHERICAL) {
		draw_tile_spherical(tile_x, tile_y, tile_wd, tile_ht, texture_id, &t);
	}
}
