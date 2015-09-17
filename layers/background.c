#include <stdbool.h>
#include <GL/gl.h>

#include "../viewport.h"
#include "../layers.h"
#include "../inlinebin.h"
#include "../programs.h"
#include "../programs/bkgd.h"

// The background layer is the diagonal pinstripe pattern that is shown in the
// out-of-bounds area of the viewport.

static struct {
	GLuint		 id;
	GLuint		 size;
	GLint		 loc;
	const char	*data;
}
tex = {
	.size = 16,
	.data =
	"///\"\"\"\37\37\37\"\"\"///###\37\37\37\"\"\"///###\37\37\37\"\"\"///###"
	"\37\37\37\"\"\"\"\"\"\37\37\37\"\"\"///###\37\37\37\"\"\"///###\37\37\37"
	"\"\"\"///###\37\37\37\"\"\"///\37\37\37\"\"\"///###\37\37\37\"\"\"///###"
	"\37\37\37\"\"\"///###\37\37\37\"\"\"///###\"\"\"///###\37\37\37\"\"\"///"
	"###\37\37\37\"\"\"///###\37\37\37\"\"\"///###\37\37\37///\"\"\"\37\37\37"
	"\"\"\"///###\37\37\37\"\"\"///###\37\37\37\"\"\"///###\37\37\37\"\"\"\"\""
	"\"\37\37\37\"\"\"///###\37\37\37\"\"\"///###\37\37\37\"\"\"///###\37\37\37"
	"\"\"\"///\37\37\37\"\"\"///###\37\37\37\"\"\"///###\37\37\37\"\"\"///###"
	"\37\37\37\"\"\"///###\"\"\"///###\37\37\37\"\"\"///###\37\37\37\"\"\"///"
	"###\37\37\37\"\"\"///###\37\37\37///\"\"\"\37\37\37\"\"\"///###\37\37\37"
	"\"\"\"///###\37\37\37\"\"\"///###\37\37\37\"\"\"\"\"\"\37\37\37\"\"\"///"
	"###\37\37\37\"\"\"///###\37\37\37\"\"\"///###\37\37\37\"\"\"///\37\37\37"
	"\"\"\"///###\37\37\37\"\"\"///###\37\37\37\"\"\"///###\37\37\37\"\"\"///"
	"###\"\"\"///###\37\37\37\"\"\"///###\37\37\37\"\"\"///###\37\37\37\"\"\""
	"///###\37\37\37///\"\"\"\37\37\37\"\"\"///###\37\37\37\"\"\"///###\37\37"
	"\37\"\"\"///###\37\37\37\"\"\"\"\"\"\37\37\37\"\"\"///###\37\37\37\"\"\""
	"///###\37\37\37\"\"\"///###\37\37\37\"\"\"///\37\37\37\"\"\"///###\37\37"
	"\37\"\"\"///###\37\37\37\"\"\"///###\37\37\37\"\"\"///###\"\"\"///###\37"
	"\37\37\"\"\"///###\37\37\37\"\"\"///###\37\37\37\"\"\"///###\37\37\37"
};

static bool
occludes (void)
{
	// The background always fully occludes:
	return true;
}

static void
paint (void)
{
	// Nothing to do if the viewport is completely within world bounds:
	if (viewport_within_world_bounds())
		return;

	float wd = (float)viewport_get_wd() / tex.size;
	float ht = (float)viewport_get_ht() / tex.size;

	// Use the background program:
	program_bkgd_use(&((struct program_bkgd) {
		.tex = 0,
	}));

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, tex.id);

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();

	// Draw the quad:
	glBegin(GL_QUADS);

	// Top left:
	glVertexAttrib2f(tex.loc, 0, ht);
	glVertex3f(-1, 1, 0.5);

	// Top right:
	glVertexAttrib2f(tex.loc, wd, ht);
	glVertex3f(1, 1, 0.5);

	// Bottom right:
	glVertexAttrib2f(tex.loc, wd, 0);
	glVertex3f(1, -1, 0.5);

	// Bottom left:
	glVertexAttrib2f(tex.loc, 0, 0);
	glVertex3f(-1, -1, 0.5);

	glEnd();

	program_none();
}

static bool
init (void)
{
	// Generate texture:
	glGenTextures(1, &tex.id);
	glBindTexture(GL_TEXTURE_2D, tex.id);

	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, tex.size, tex.size, 0, GL_RGB, GL_UNSIGNED_BYTE, tex.data);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

	tex.loc = program_bkgd_loc_texture();

	return true;
}

struct layer *
layer_background (void)
{
	static struct layer layer = {
		.init     = &init,
		.occludes = &occludes,
		.paint    = &paint,
	};

	return &layer;
}
