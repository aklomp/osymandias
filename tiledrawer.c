#include <stdbool.h>
#include <math.h>

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
draw_tile_planar (GLuint texture_id, const struct texture *t, float p[4][3])
{
	GLdouble txoffs = (GLdouble)t->offset_x / 256.0;
	GLdouble tyoffs = (GLdouble)t->offset_y / 256.0;
	GLdouble twd = (GLdouble)t->wd / 256.0;
	GLdouble tht = (GLdouble)t->ht / 256.0;

	glBindTexture(GL_TEXTURE_2D, texture_id);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

	glBegin(GL_QUADS);
		glTexCoord2f(txoffs,       tyoffs);       glVertex2f(p[0][0], p[0][1]);
		glTexCoord2f(txoffs + twd, tyoffs);       glVertex2f(p[1][0], p[1][1]);
		glTexCoord2f(txoffs + twd, tyoffs + tht); glVertex2f(p[2][0], p[2][1]);
		glTexCoord2f(txoffs,       tyoffs + tht); glVertex2f(p[3][0], p[3][1]);
	glEnd();
}

static void
draw_tile_spherical (int tile_x, int tile_y, int tile_wd, int tile_ht, double cx, double cy, GLuint texture_id, const struct texture *t)
{
	double x[4], y[4], z[4];
	double sinlat, coslat;
	unsigned int world_size = world_get_size();
	unsigned int world_zoom = world_get_zoom();

	// Calculate tilt angle in radians:
	double lon = world_x_to_lon(cx, world_size);
	double lat = world_y_to_lat(cy, world_size);

	sincos(lat, &sinlat, &coslat);

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

	// lon1, lon2, lat1, and lat2 form the cornerpoints of the quad being
	// drawn. The variable use is non-obvious, but we reuse lon2/lat2's
	// coordinates as the lon1/lat1 for the next round.

	double lon1 = world_x_to_lon(tile_x, world_size);
	for (int dx = 0; dx < xdivide; dx++)
	{
		double lon2 = world_x_to_lon(tile_x + (double)((dx + 1) * tile_wd) / xdivide, world_size);
		double lat1 = world_y_to_lat(tile_y, world_size);
		for (int dy = 0; dy < ydivide; dy++)
		{
			double lat2 = world_y_to_lat(tile_y + (double)((dy + 1) * tile_ht) / ydivide, world_size);

			GLdouble txoffs = (GLdouble)t->offset_x / 256.0 + dx * twd;
			GLdouble tyoffs = (GLdouble)t->offset_y / 256.0 + dy * tht;

			latlon_to_xyz(lat1, lon1, world_size, lon, sinlat, coslat, &x[0], &y[0], &z[0]);
			latlon_to_xyz(lat1, lon2, world_size, lon, sinlat, coslat, &x[1], &y[1], &z[1]);
			latlon_to_xyz(lat2, lon2, world_size, lon, sinlat, coslat, &x[2], &y[2], &z[2]);
			latlon_to_xyz(lat2, lon1, world_size, lon, sinlat, coslat, &x[3], &y[3], &z[3]);

			glNormal3f(x[0], y[0], z[0]); glTexCoord2f(txoffs,       tyoffs);       glVertex3f(x[0], y[0], z[0]);
			glNormal3f(x[1], y[1], z[1]); glTexCoord2f(txoffs + twd, tyoffs);       glVertex3f(x[1], y[1], z[1]);
			glNormal3f(x[2], y[2], z[2]); glTexCoord2f(txoffs + twd, tyoffs + tht); glVertex3f(x[2], y[2], z[2]);
			glNormal3f(x[3], y[3], z[3]); glTexCoord2f(txoffs,       tyoffs + tht); glVertex3f(x[3], y[3], z[3]);

			lat1 = lat2;
		}
		lon1 = lon2;
	}
	glEnd();
}

void
tiledrawer (int tile_x, int tile_y, int tile_wd, int tile_ht, double cx, double cy, GLuint texture_id, const struct quadtree_req *req, float p[4][3])
{
	struct texture t;

	// If we couldn't find the texture at the requested resolution and
	// had to settle for a lower-res one, we need to cut out the proper
	// sub-block from the texture. Calculate coords of the block we need:
	cutout_texture(tile_x, tile_y, tile_wd, tile_ht, req, &t);

	// With this cutout information in hand, draw the actual tile:
	if (viewport_mode_get() == VIEWPORT_MODE_PLANAR) {
		draw_tile_planar(texture_id, &t, p);
	}
	if (viewport_mode_get() == VIEWPORT_MODE_SPHERICAL) {
		draw_tile_spherical(tile_x, tile_y, tile_wd, tile_ht, cx, cy, texture_id, &t);
	}
}
