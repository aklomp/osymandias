#include <stdbool.h>
#include <math.h>

#include <GL/gl.h>

#include "quadtree.h"
#include "coord3d.h"
#include "world.h"
#include "viewport.h"
#include "tiledrawer.h"

struct texture {
	float wd;
	float ht;
	float offset_x;
	float offset_y;
};

struct vector {
	float x;
	float y;
	float z;
	float w;
} __attribute__((packed));

static void
cutout_texture (const struct tiledrawer *tile, struct texture *tex)
{
	unsigned int zoomdiff = tile->req->world_zoom - tile->req->found_zoom;

	// This is the nth block out of parent, counting from top left:
	float xblock = fmodf(tile->x, (1 << zoomdiff));
	float yblock = fmodf(tile->y, (1 << zoomdiff));

	// Multiplication before division, avoid clipping:
	tex->offset_x = ldexpf(xblock, 8 - zoomdiff);
	tex->offset_y = ldexpf(yblock, 8 - zoomdiff);
	tex->wd = ldexpf(tile->wd, 8 - zoomdiff);
	tex->ht = ldexpf(tile->ht, 8 - zoomdiff);
}

static void
draw_tile_planar (const struct tiledrawer *tile, const struct texture *tex)
{
	struct vector *pos = (struct vector *)tile->pos;

	GLdouble txoffs = (GLdouble)ldexpf(tex->offset_x, -8);
	GLdouble tyoffs = (GLdouble)ldexpf(tex->offset_y, -8);
	GLdouble twd = (GLdouble)ldexpf(tex->wd, -8);
	GLdouble tht = (GLdouble)ldexpf(tex->ht, -8);

	glBindTexture(GL_TEXTURE_2D, tile->texture_id);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

	glBegin(GL_QUADS);
		glTexCoord2f(txoffs,       tyoffs);       glVertex2fv(&pos[0].x);
		glTexCoord2f(txoffs + twd, tyoffs);       glVertex2fv(&pos[1].x);
		glTexCoord2f(txoffs + twd, tyoffs + tht); glVertex2fv(&pos[2].x);
		glTexCoord2f(txoffs,       tyoffs + tht); glVertex2fv(&pos[3].x);
	glEnd();
}

static void
draw_tile_spherical (const struct tiledrawer *tile, const struct texture *tex)
{
	struct vector *pos = (struct vector *)tile->pos;

	GLdouble txoffs = (GLdouble)ldexpf(tex->offset_x, -8);
	GLdouble tyoffs = (GLdouble)ldexpf(tex->offset_y, -8);
	GLdouble twd = (GLdouble)ldexpf(tex->wd, -8);
	GLdouble tht = (GLdouble)ldexpf(tex->ht, -8);

	glBindTexture(GL_TEXTURE_2D, tile->texture_id);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

	glBegin(GL_QUADS);
		glNormal3fv(&pos[0].x); glTexCoord2f(txoffs,       tyoffs);       glVertex3fv(&pos[0].x);
		glNormal3fv(&pos[1].x); glTexCoord2f(txoffs + twd, tyoffs);       glVertex3fv(&pos[1].x);
		glNormal3fv(&pos[2].x); glTexCoord2f(txoffs + twd, tyoffs + tht); glVertex3fv(&pos[2].x);
		glNormal3fv(&pos[3].x); glTexCoord2f(txoffs,       tyoffs + tht); glVertex3fv(&pos[3].x);
	glEnd();
}

void
tiledrawer (const struct tiledrawer *tile)
{
	struct texture tex;

	// If we couldn't find the texture at the requested resolution and
	// had to settle for a lower-res one, we need to cut out the proper
	// sub-block from the texture. Calculate coords of the block we need:
	cutout_texture(tile, &tex);

	// With this cutout information in hand, draw the actual tile:
	switch (viewport_mode_get())
	{
	case VIEWPORT_MODE_PLANAR:
		draw_tile_planar(tile, &tex);
		break;

	case VIEWPORT_MODE_SPHERICAL:
		draw_tile_spherical(tile, &tex);
		break;
	}
}
