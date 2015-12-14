#include <stdbool.h>
#include <math.h>

#include <GL/gl.h>

#include "worlds.h"
#include "tiledrawer.h"
#include "vector.h"

struct texture {
	float wd;
	float ht;
	struct {
		float x;
		float y;
	} offset;
};

void
tiledrawer (const struct tiledrawer *tile)
{
	struct texture tex;

	unsigned int zoomdiff = tile->zoom.world - tile->zoom.found;

	// If we couldn't find the texture at the requested resolution and
	// had to settle for a lower-res one, we need to cut out the proper
	// sub-block from the texture. Calculate coords of the block we need:

	// This is the nth block out of parent, counting from top left:
	float xblock = fmodf(tile->x, (1 << zoomdiff));
	float yblock = fmodf(tile->y, (1 << zoomdiff));

	// Multiplication before division, avoid clipping:
	tex.offset.x = ldexpf(xblock, 8 - zoomdiff);
	tex.offset.y = ldexpf(yblock, 8 - zoomdiff);
	tex.wd = ldexpf(tile->wd, 8 - zoomdiff);
	tex.ht = ldexpf(tile->ht, 8 - zoomdiff);

	float txoffs = ldexpf(tex.offset.x, -8);
	float tyoffs = ldexpf(tex.offset.y, -8);
	float twd = ldexpf(tex.wd, -8);
	float tht = ldexpf(tex.ht, -8);

	struct {
		float x;
		float y;
	} __attribute__((packed))
	texcoord[4] = {
		{ txoffs,       tyoffs       },
		{ txoffs + twd, tyoffs       },
		{ txoffs + twd, tyoffs + tht },
		{ txoffs,       tyoffs + tht },
	};

	// Bind texture:
	glBindTexture(GL_TEXTURE_2D, tile->texture_id);

	// Submit normal, texture and space coords for vertex:
	glBegin(GL_QUADS);
	for (int i = 0; i < 4; i++) {
		glNormal3fv((float *)&tile->normal[i]);
		glTexCoord2fv((float *)&texcoord[i]);
		glVertex3fv((float *)&tile->coords[i]);
	}
	glEnd();
}
