#include <stdbool.h>
#include <math.h>

#include <GL/gl.h>

#include "quadtree.h"
#include "coord3d.h"
#include "world.h"
#include "viewport.h"

struct texture {
	float wd;
	float ht;
	float offset_x;
	float offset_y;
};

static void
cutout_texture (float orig_x, float orig_y, float wd, float ht, const struct quadtree_req *req, struct texture *t)
{
	unsigned int zoomdiff = req->world_zoom - req->found_zoom;

	// This is the nth block out of parent, counting from top left:
	float xblock = fmodf(orig_x, (1 << zoomdiff));
	float yblock = fmodf(orig_y, (1 << zoomdiff));

	// Multiplication before division, avoid clipping:
	t->offset_x = ldexpf(xblock, 8 - zoomdiff);
	t->offset_y = ldexpf(yblock, 8 - zoomdiff);
	t->wd = ldexpf(wd, 8 - zoomdiff);
	t->ht = ldexpf(ht, 8 - zoomdiff);
}

static void
draw_tile_planar (GLuint texture_id, const struct texture *t, float p[4][3])
{
	GLdouble txoffs = (GLdouble)ldexpf(t->offset_x, -8);
	GLdouble tyoffs = (GLdouble)ldexpf(t->offset_y, -8);
	GLdouble twd = (GLdouble)ldexpf(t->wd, -8);
	GLdouble tht = (GLdouble)ldexpf(t->ht, -8);

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
draw_tile_spherical (GLuint texture_id, const struct texture *t, float p[4][3])
{
	GLdouble txoffs = (GLdouble)ldexpf(t->offset_x, -8);
	GLdouble tyoffs = (GLdouble)ldexpf(t->offset_y, -8);
	GLdouble twd = (GLdouble)ldexpf(t->wd, -8);
	GLdouble tht = (GLdouble)ldexpf(t->ht, -8);

	glBindTexture(GL_TEXTURE_2D, texture_id);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

	glBegin(GL_QUADS);
		glNormal3f(p[0][0], p[0][1], p[0][2]); glTexCoord2f(txoffs,       tyoffs);       glVertex3f(p[0][0], p[0][1], p[0][2]);
		glNormal3f(p[1][0], p[1][1], p[1][2]); glTexCoord2f(txoffs + twd, tyoffs);       glVertex3f(p[1][0], p[1][1], p[1][2]);
		glNormal3f(p[2][0], p[2][1], p[2][2]); glTexCoord2f(txoffs + twd, tyoffs + tht); glVertex3f(p[2][0], p[2][1], p[2][2]);
		glNormal3f(p[3][0], p[3][1], p[3][2]); glTexCoord2f(txoffs,       tyoffs + tht); glVertex3f(p[3][0], p[3][1], p[3][2]);
	glEnd();
}

void
tiledrawer (float tile_x, float tile_y, float tile_wd, float tile_ht, GLuint texture_id, const struct quadtree_req *req, float p[4][3])
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
		draw_tile_spherical(texture_id, &t, p);
	}
}
