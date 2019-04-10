#include <stdbool.h>
#include <string.h>
#include <math.h>
#include <GL/gl.h>

#include "../matrix.h"
#include "../worlds.h"
#include "../viewport.h"
#include "../camera.h"
#include "../layers.h"
#include "../tilepicker.h"
#include "../glutil.h"
#include "../programs.h"
#include "../programs/frustum.h"
#include "../programs/solid.h"
#include "../util.h"

#define SIZE	256.0f
#define MARGIN	 10.0f

// Projection matrix:
static struct {
	float proj[16];
} matrix;

// Vertex array and buffer objects:
static GLuint vao[3], vbo[2];
static GLuint *vao_bkgd    = &vao[0];
static GLuint *vbo_bkgd    = &vbo[0];
static GLuint *vao_tiles   = &vao[1];
static GLuint *vbo_tiles   = &vbo[1];
static GLuint *vao_frustum = &vao[2];

// Each point has 2D space coordinates and RGBA color:
struct vertex {
	struct {
		float x;
		float y;
	} coords;
	struct {
		float r;
		float g;
		float b;
		float a;
	} color;
} __attribute__((packed));

static inline void
setup_viewport (void)
{
	int xpos = viewport_get_wd() - MARGIN - SIZE;
	int ypos = viewport_get_ht() - MARGIN - SIZE;

	glLineWidth(1.0);
	glViewport(xpos, ypos, SIZE, SIZE);

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	glDisable(GL_DEPTH_TEST);
}

static void
paint_background (GLuint vao)
{
	// Draw solid background:
	glBindVertexArray(vao);
	glBindBuffer(GL_ARRAY_BUFFER, *vbo_bkgd);
	glutil_draw_quad();
}

static void
paint_tiles (void)
{
	static float zoomcolors[6][4] = {
		{ 1.0f, 0.0f, 0.0f, 0.5f },
		{ 0.0f, 1.0f, 0.0f, 0.5f },
		{ 0.0f, 0.0f, 1.0f, 0.5f },
		{ 0.5f, 0.5f, 0.0f, 0.5f },
		{ 0.0f, 0.5f, 0.5f, 0.5f },
		{ 0.5f, 0.0f, 0.5f, 0.5f },
	};

	static float white[4] =
		{ 1.0f, 1.0f, 1.0f, 0.5f };

	// Draw 50 tiles (200 vertices) at a time:
	struct tile {
		struct vertex vertex[4];
	} __attribute__((packed))
	tile[50];

	// 50 tiles have 100 triangles with 3 vertices each:
	struct {
		struct {
			GLubyte a;
			GLubyte b;
			GLubyte c;
		} __attribute__((packed))
		tri[2];
	} __attribute__((packed))
	index[50];

	// Populate indices with two triangles (0-1-3 and 1-2-3):
	for (int t = 0; t < 50; t++) {
		index[t].tri[0].a = t * 4 + 0;
		index[t].tri[0].b = t * 4 + 1;
		index[t].tri[0].c = t * 4 + 3;

		index[t].tri[1].a = t * 4 + 1;
		index[t].tri[1].b = t * 4 + 2;
		index[t].tri[1].c = t * 4 + 3;
	}

	// First draw tiles with solid background:
	const struct tilepicker *tptile = tilepicker_first();

	while (tptile)
	{
		int t;

		// Fetch 50 tiles at most:
		for (t = 0; tptile && t < 50; t++)
		{
			// Bottom left:
			tile[t].vertex[0].coords.x = tptile->x;
			tile[t].vertex[0].coords.y = tptile->y;

			// Bottom right:
			tile[t].vertex[1].coords.x = tptile->x + 1;
			tile[t].vertex[1].coords.y = tptile->y;

			// Top right:
			tile[t].vertex[2].coords.x = tptile->x + 1;
			tile[t].vertex[2].coords.y = tptile->y + 1;

			// Top left:
			tile[t].vertex[3].coords.x = tptile->x;
			tile[t].vertex[3].coords.y = tptile->y + 1;

			// Scale:
			for (int i = 0; i < 4; i++) {
				tile[t].vertex[i].coords.x = ldexpf(tile[t].vertex[i].coords.x, -tptile->zoom);
				tile[t].vertex[i].coords.y = 1.0f - ldexpf(tile[t].vertex[i].coords.y, -tptile->zoom);
			}

			// Solid fill color:
			float *color = zoomcolors[tptile->zoom % 3];
			for (int i = 0; i < 4; i++)
				memcpy(&tile[t].vertex[i].color, color, sizeof(tile[t].vertex[i].color));

			tptile = tilepicker_next();
		}

		// Upload vertex data:
		glBindBuffer(GL_ARRAY_BUFFER, *vbo_tiles);
		glBufferData(GL_ARRAY_BUFFER, sizeof(struct tile) * t, tile, GL_STREAM_DRAW);

		// Draw indices:
		glBindVertexArray(*vao_tiles);
		glDrawElements(GL_TRIANGLES, t * 6, GL_UNSIGNED_BYTE, index);

		// Change colors to white:
		float *color = white;
		for (int i = 0; i < t; i++)
			for (int v = 0; v < 4; v++)
				memcpy(&tile[i].vertex[v].color, color, sizeof(tile[i].vertex[v].color));

		// Upload modified data:
		glBufferData(GL_ARRAY_BUFFER, sizeof(struct tile) * t, tile, GL_STREAM_DRAW);

		// Draw as line loops:
		for (int i = 0; i < t; i++)
			glDrawArrays(GL_LINE_LOOP, i * 4, 4);
	}
}

static void
paint (const struct camera *cam)
{
	// Draw 1:1 to screen coordinates, origin bottom left:
	setup_viewport();

	// Paint background using solid program:
	program_solid_use(&((struct program_solid) {
		.matrix = matrix.proj,
	}));

	paint_background(*vao_bkgd);

	// Paint tiles using solid program:
	paint_tiles();

	// Paint background using frustum program:
	program_frustum_use(&((struct program_frustum) {
		.mat_proj      = matrix.proj,
		.mat_viewproj  = cam->matrix.viewproj,
		.mat_model     = world_get_matrix(),
		.mat_view_inv  = cam->matrix.inverse.view,
		.mat_model_inv = world_get_matrix_inverse(),
	}));

	paint_background(*vao_frustum);

	// Reset program:
	program_none();

	glDisable(GL_BLEND);

	// Reset program:
	program_none();
}

#define OFFSET_COORDS	((void *) offsetof(struct vertex, coords))
#define OFFSET_COLOR	((void *) offsetof(struct vertex, color))

static void
add_pointer (unsigned int loc, int size, const void *ptr)
{
	glEnableVertexAttribArray(loc);
	glVertexAttribPointer(loc, size, GL_FLOAT, GL_FALSE, sizeof (struct vertex), ptr);
}

static void
init_bkgd (void)
{
	// Background quad is array of counterclockwise vertices:
	//
	//   3--2
	//   |  |
	//   0--1
	//
	static const struct vertex bkgd[4] = {
		{ { 0.0f, 0.0f }, { 0.3f, 0.3f, 0.3f, 0.5f } },
		{ { 1.0f, 0.0f }, { 0.3f, 0.3f, 0.3f, 0.5f } },
		{ { 1.0f, 1.0f }, { 0.3f, 0.3f, 0.3f, 0.5f } },
		{ { 0.0f, 1.0f }, { 0.3f, 0.3f, 0.3f, 0.5f } },
	};

	// Bind buffer and vertex array, upload vertices:
	glBindBuffer(GL_ARRAY_BUFFER, *vbo_bkgd);
	glBindVertexArray(*vao_bkgd);
	glBufferData(GL_ARRAY_BUFFER, sizeof (bkgd), bkgd, GL_STATIC_DRAW);

	// Add pointer to 'vertex' and 'color' attributes:
	add_pointer(program_solid_loc_vertex(), 2, OFFSET_COORDS);
	add_pointer(program_solid_loc_color(),  4, OFFSET_COLOR);

	// For the orthogonal projection matrix, define the bounds such that
	// the projected area is one pixel larger on all sides than the actual
	// size in pixels. This will allow the tile outlines to be drawn
	// without being clipped.
	const float min = -1.0f / SIZE;
	const float max = (SIZE + 1.0f) / SIZE;

	mat_ortho(matrix.proj, min, max, min, max, 0, 1);
}

static void
init_frustum (void)
{
	// Bind buffer and vertex array (reuse the background quad):
	glBindBuffer(GL_ARRAY_BUFFER, *vbo_bkgd);
	glBindVertexArray(*vao_frustum);

	// Add pointer to 'vertex' attribute:
	add_pointer(program_frustum_loc_vertex(), 2, OFFSET_COORDS);
}

static void
init_tiles (void)
{
	// Bind buffer and vertex array:
	glBindBuffer(GL_ARRAY_BUFFER, *vbo_tiles);
	glBindVertexArray(*vao_tiles);

	// Add pointer to 'vertex' and 'color' attributes:
	add_pointer(program_solid_loc_vertex(), 2, OFFSET_COORDS);
	add_pointer(program_solid_loc_color(),  4, OFFSET_COLOR);
}

static bool
init (void)
{
	// Generate vertex buffer and array objects:
	glGenBuffers(NELEM(vbo), vbo);
	glGenVertexArrays(NELEM(vao), vao);

	init_bkgd();
	init_frustum();
	init_tiles();

	return true;
}

static void
destroy (void)
{
	// Delete vertex array and buffer objects:
	glDeleteVertexArrays(NELEM(vao), vao);
	glDeleteBuffers(NELEM(vbo), vbo);
}

// Export public methods:
LAYER(50) = {
	.init    = &init,
	.paint   = &paint,
	.destroy = &destroy,
};
