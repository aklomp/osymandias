#include <stdbool.h>
#include <math.h>

#include <GL/gl.h>

#include "quadtree.h"
#include "world.h"
#include "tiledrawer.h"
#include "vector.h"

struct texture {
	float wd;
	float ht;
	float offset_x;
	float offset_y;
};

void
tiledrawer (const struct tiledrawer *tile)
{
	struct texture tex;

	unsigned int zoomdiff = tile->req->world_zoom - tile->req->found_zoom;

	// If we couldn't find the texture at the requested resolution and
	// had to settle for a lower-res one, we need to cut out the proper
	// sub-block from the texture. Calculate coords of the block we need:

	// This is the nth block out of parent, counting from top left:
	float xblock = fmodf(tile->x, (1 << zoomdiff));
	float yblock = fmodf(tile->y, (1 << zoomdiff));

	// Multiplication before division, avoid clipping:
	tex.offset_x = ldexpf(xblock, 8 - zoomdiff);
	tex.offset_y = ldexpf(yblock, 8 - zoomdiff);
	tex.wd = ldexpf(tile->wd, 8 - zoomdiff);
	tex.ht = ldexpf(tile->ht, 8 - zoomdiff);

	GLdouble txoffs = (GLdouble)ldexpf(tex.offset_x, -8);
	GLdouble tyoffs = (GLdouble)ldexpf(tex.offset_y, -8);
	GLdouble twd = (GLdouble)ldexpf(tex.wd, -8);
	GLdouble tht = (GLdouble)ldexpf(tex.ht, -8);

	glBindTexture(GL_TEXTURE_2D, tile->texture_id);

	glBegin(GL_QUADS);
		glNormal3fv((float *)&tile->normal[0]); glTexCoord2f(txoffs,       tyoffs);       glVertex3fv((float *)&tile->coords[0]);
		glNormal3fv((float *)&tile->normal[1]); glTexCoord2f(txoffs + twd, tyoffs);       glVertex3fv((float *)&tile->coords[1]);
		glNormal3fv((float *)&tile->normal[2]); glTexCoord2f(txoffs + twd, tyoffs + tht); glVertex3fv((float *)&tile->coords[2]);
		glNormal3fv((float *)&tile->normal[3]); glTexCoord2f(txoffs,       tyoffs + tht); glVertex3fv((float *)&tile->coords[3]);
	glEnd();
}
