#include <stdbool.h>
#include <string.h>
#include <math.h>
#include <GL/gl.h>

#include "../matrix.h"
#include "../viewport.h"
#include "../layer.h"
#include "../tilepicker.h"
#include "../glutil.h"
#include "../program.h"
#include "../program/frustum.h"
#include "../program/solid.h"
#include "../util.h"

#define MARGIN 40.0f

// Forward declaration.
static struct layer layer;

// Projection matrix:
static struct {
	float  proj32[16];
	double proj64[16];
} matrix;

// Vertex array and buffer objects:
static struct {
	uint32_t vao[3];
	uint32_t vbo[2];

	// Grey background patch:
	struct {
		uint32_t *vao;
		uint32_t *vbo;
	} bkgd;

	// Overdrawn tiles:
	struct {
		uint32_t *vao;
		uint32_t *vbo;
	} tiles;

	// Overdrawn frustum extent:
	struct {
		uint32_t *vao;
	} frustum;

	// Screen position in pixels:
	struct {
		uint32_t x;
		uint32_t y;
	} pos;

	uint32_t size;
}
state = {
	.bkgd.vao    = &state.vao[0],
	.bkgd.vbo    = &state.vbo[0],
	.tiles.vao   = &state.vao[1],
	.tiles.vbo   = &state.vbo[1],
	.frustum.vao = &state.vao[2],
};

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

static void
on_resize (const struct viewport *vp)
{
	// Fit square to smallest screen dimension:
	state.size = (vp->width < vp->height ? vp->width : vp->height) - 2 * MARGIN;
	state.pos.x = (vp->width - state.size) / 2;
	state.pos.y = (vp->height - state.size) / 2;

	// For the orthogonal projection matrix, define the bounds such that
	// the projected area is one pixel larger than the actual size in
	// pixels. This will allow the tile outlines to be drawn without being
	// clipped.
	const double min = -0.5 / state.size;
	const double max = (state.size + 0.5) / state.size;

	mat_ortho(matrix.proj64, min, max, min, max, 0, 1);
	mat_to_float(matrix.proj32, matrix.proj64);
}

static void
paint_background (GLuint vao)
{
	// Draw solid background:
	glBindVertexArray(vao);
	glBindBuffer(GL_ARRAY_BUFFER, *state.bkgd.vbo);
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
		glBindBuffer(GL_ARRAY_BUFFER, *state.tiles.vbo);
		glBufferData(GL_ARRAY_BUFFER, sizeof(struct tile) * t, tile, GL_STREAM_DRAW);

		// Draw indices:
		glBindVertexArray(*state.tiles.vao);
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
on_paint (const struct camera *cam, const struct viewport *vp)
{
	(void) cam;

	// Draw 1:1 to screen coordinates, origin bottom left:
	glLineWidth(1.0);
	glViewport(state.pos.x, state.pos.y, state.size, state.size);

	glDisable(GL_DEPTH_TEST);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	// Paint background using solid program:
	program_solid_use(&((struct program_solid) {
		.matrix = matrix.proj32,
	}));

	paint_background(*state.bkgd.vao);

	// Paint tiles using solid program:
	paint_tiles();

	// Paint background using frustum program:
	program_frustum_use(&((struct program_frustum) {
		.cam            = vp->cam_pos,
		.mat_mvp_origin = vp->matrix32.modelviewproj_origin,
		.mat_proj       = matrix.proj32,
	}));

	paint_background(*state.frustum.vao);

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
		{ { 0.0f, 0.0f }, { 0.3f, 0.3f, 0.3f, 0.3f } },
		{ { 1.0f, 0.0f }, { 0.3f, 0.3f, 0.3f, 0.3f } },
		{ { 1.0f, 1.0f }, { 0.3f, 0.3f, 0.3f, 0.3f } },
		{ { 0.0f, 1.0f }, { 0.3f, 0.3f, 0.3f, 0.3f } },
	};

	// Bind buffer and vertex array, upload vertices:
	glBindBuffer(GL_ARRAY_BUFFER, *state.bkgd.vbo);
	glBindVertexArray(*state.bkgd.vao);
	glBufferData(GL_ARRAY_BUFFER, sizeof (bkgd), bkgd, GL_STATIC_DRAW);

	// Add pointer to 'vertex' and 'color' attributes:
	add_pointer(program_solid_loc_vertex(), 2, OFFSET_COORDS);
	add_pointer(program_solid_loc_color(),  4, OFFSET_COLOR);
}

static void
init_frustum (void)
{
	// Bind buffer and vertex array (reuse the background quad):
	glBindBuffer(GL_ARRAY_BUFFER, *state.bkgd.vbo);
	glBindVertexArray(*state.frustum.vao);

	// Add pointer to 'vertex' attribute:
	add_pointer(program_frustum_loc_vertex(), 2, OFFSET_COORDS);
}

static void
init_tiles (void)
{
	// Bind buffer and vertex array:
	glBindBuffer(GL_ARRAY_BUFFER, *state.tiles.vbo);
	glBindVertexArray(*state.tiles.vao);

	// Add pointer to 'vertex' and 'color' attributes:
	add_pointer(program_solid_loc_vertex(), 2, OFFSET_COORDS);
	add_pointer(program_solid_loc_color(),  4, OFFSET_COLOR);
}

static bool
on_init (const struct viewport *vp)
{
	(void) vp;

	// Generate vertex buffer and array objects:
	glGenBuffers(NELEM(state.vbo), state.vbo);
	glGenVertexArrays(NELEM(state.vao), state.vao);

	init_bkgd();
	init_frustum();
	init_tiles();

	return true;
}

static void
on_destroy (void)
{
	// Delete vertex array and buffer objects:
	glDeleteVertexArrays(NELEM(state.vao), state.vao);
	glDeleteBuffers(NELEM(state.vbo), state.vbo);
}

void layer_overview_toggle_visible (void)
{
	layer.visible ^= 1;
}

static struct layer layer = {
	.name       = "Overview",
	.zdepth     = 50,
	.visible    = false,
	.on_init    = &on_init,
	.on_paint   = &on_paint,
	.on_resize  = &on_resize,
	.on_destroy = &on_destroy,
};

LAYER_REGISTER(&layer)
