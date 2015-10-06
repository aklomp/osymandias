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

// Array of vertices. The vertices are laid out as follows (two triangles,
// clockwise rotation):
//
//   0--1    3
//   | /    /|
//   |/    / |
//   2    5--4
//
// Points (1, 3) and (2, 5) are identical.

static struct vertex {
	float x;
	float y;
	float u;
	float v;
}
vertex[6];

static GLuint vao, vbo;

static bool
occludes (void)
{
	// The background always fully occludes:
	return true;
}

static void
vertcoords (void)
{
	// Top left:
	vertex[0].x = -1.0;
	vertex[0].y =  1.0;

	// Top right:
	vertex[1].x = vertex[3].x = 1.0;
	vertex[1].y = vertex[3].y = 1.0;

	// Bottom right:
	vertex[4].x =  1.0;
	vertex[4].y = -1.0;

	// Bottom left:
	vertex[2].x = vertex[5].x = -1.0;
	vertex[2].y = vertex[5].y = -1.0;
}

static void
texcoords (float screen_wd, float screen_ht)
{
	float wd = screen_wd / tex.size;
	float ht = screen_ht / tex.size;

	// Top left:
	vertex[0].u = 0;
	vertex[0].v = ht;

	// Top right:
	vertex[1].u = vertex[3].u = wd;
	vertex[1].v = vertex[3].v = ht;

	// Bottom right:
	vertex[4].u = wd;
	vertex[4].v = 0;

	// Bottom left:
	vertex[2].u = vertex[5].u = 0;
	vertex[2].v = vertex[5].v = 0;
}

static void
paint (void)
{
	// Nothing to do if the viewport is completely within world bounds:
	if (viewport_within_world_bounds())
		return;

	// Update texture coordinates:
	texcoords(viewport_get_wd(), viewport_get_ht());

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

	// Copy vertices to buffer:
	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertex), vertex, GL_STATIC_DRAW);

	// Draw all triangles in the buffer:
	glBindVertexArray(vao);
	glDrawArrays(GL_TRIANGLES, 0, sizeof(vertex) / sizeof(vertex[0]));

	program_none();
}

static bool
init (void)
{
	// Populate vertex coordinates:
	vertcoords();

	// Generate vertex buffer object:
	glGenBuffers(1, &vbo);

	// Generate vertex array object:
	glGenVertexArrays(1, &vao);

	// Bind buffer and vertex array:
	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	glBindVertexArray(vao);

	// Add pointer to 'vertex' attribute:
	glEnableVertexAttribArray(program_bkgd_loc_vertex());
	glVertexAttribPointer(program_bkgd_loc_vertex(), 2, GL_FLOAT, GL_FALSE,
		sizeof(struct vertex),
		(void *)(&((struct vertex *)0)->x));

	// Add pointer to 'texture' attribute:
	glEnableVertexAttribArray(program_bkgd_loc_texture());
	glVertexAttribPointer(program_bkgd_loc_texture(), 2, GL_FLOAT, GL_FALSE,
		sizeof(struct vertex),
		(void *)(&((struct vertex *)0)->u));

	// Generate texture:
	glGenTextures(1, &tex.id);
	glBindTexture(GL_TEXTURE_2D, tex.id);

	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, tex.size, tex.size, 0, GL_RGB, GL_UNSIGNED_BYTE, tex.data);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

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
