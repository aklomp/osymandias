#include <stdbool.h>
#include <GL/gl.h>

#include "../vector.h"
#include "../matrix.h"
#include "../world.h"
#include "../viewport.h"
#include "../camera.h"
#include "../layers.h"
#include "../tilepicker.h"
#include "../inlinebin.h"
#include "../programs.h"
#include "../programs/frustum.h"
#include "../programs/solid.h"

#define SIZE	256.0f
#define MARGIN	 10.0f

// Projection matrix:
static float mat_proj[16];

static GLuint vao_bkgd, vao_tiles;
static GLuint vbo_bkgd, vbo_tiles;
static GLuint vao_frustum;

// Each point has 2D space coordinates and color:
struct vertex {
	float x;
	float y;
	float r;
	float g;
	float b;
	float a;
} __attribute__((packed));

static bool
occludes (void)
{
	// The overview never occludes:
	return false;
}

static inline void
setup_viewport (void)
{
	int xpos = viewport_get_wd() - MARGIN - SIZE;
	int ypos = viewport_get_ht() - MARGIN - SIZE;

	glLineWidth(1.0);
	glViewport(xpos, ypos, SIZE, SIZE);

	glMatrixMode(GL_PROJECTION);
	glLoadMatrixf(mat_proj);

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();

	glDisable(GL_DEPTH_TEST);
	glDepthMask(GL_FALSE);

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}

static void
paint_background (GLuint vao)
{
	// Array of indices. We define two counterclockwise triangles:
	// 0-1-3 and 1-2-3
	GLubyte index[6] = {
		0, 1, 3,
		1, 2, 3,
	};

	// Draw solid background:
	glBindVertexArray(vao);
	glBindBuffer(GL_ARRAY_BUFFER, vbo_bkgd);
	glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_BYTE, index);
}

static void
paint_tiles (void)
{
	static float zoomcolors[6][3] = {
		{ 1.0f, 0.0f, 0.0f },
		{ 0.0f, 1.0f, 0.0f },
		{ 0.0f, 0.0f, 1.0f },
		{ 0.5f, 0.5f, 0.0f },
		{ 0.0f, 0.5f, 0.5f },
		{ 0.5f, 0.0f, 0.5f }
	};

	int zoom;
	float x, y, tile_wd, tile_ht, pos[4][4], normal[4][4];

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

	double world_size = world_get_size();

	// First draw tiles with solid background:
	bool iter = tilepicker_first(&x, &y, &tile_wd, &tile_ht, &zoom, pos, normal);

	while (iter)
	{
		int t;

		// Fetch 50 tiles at most:
		for (t = 0; iter && t < 50; t++)
		{
			// Bottom left:
			tile[t].vertex[0].x = x;
			tile[t].vertex[0].y = world_size - y;

			// Bottom right:
			tile[t].vertex[1].x = x + tile_wd;
			tile[t].vertex[1].y = world_size - y;

			// Top right:
			tile[t].vertex[2].x = x + tile_wd;
			tile[t].vertex[2].y = world_size - y - tile_ht;

			// Top left:
			tile[t].vertex[3].x = x;
			tile[t].vertex[3].y = world_size - y - tile_ht;

			// Solid fill color:
			float r = zoomcolors[zoom % 3][0];
			float g = zoomcolors[zoom % 3][1];
			float b = zoomcolors[zoom % 3][2];
			float a = 0.5f;

			for (int i = 0; i < 4; i++) {
				tile[t].vertex[i].r = r;
				tile[t].vertex[i].g = g;
				tile[t].vertex[i].b = b;
				tile[t].vertex[i].a = a;
			}

			iter = tilepicker_next(&x, &y, &tile_wd, &tile_ht, &zoom, pos, normal);
		}

		// Upload vertex data:
		glBindBuffer(GL_ARRAY_BUFFER, vbo_tiles);
		glBufferData(GL_ARRAY_BUFFER, sizeof(struct tile) * t, tile, GL_STREAM_DRAW);

		// Draw indices:
		glBindVertexArray(vao_tiles);
		glDrawElements(GL_TRIANGLES, t * 6, GL_UNSIGNED_BYTE, index);

		// Change color to white:
		for (int i = 0; i < t; i++) {
			for (int v = 0; v < 4; v++) {
				tile[i].vertex[v].r = 1.0f;
				tile[i].vertex[v].g = 1.0f;
				tile[i].vertex[v].b = 1.0f;
				tile[i].vertex[v].a = 0.5f;
			}
		}

		// Upload modified data:
		glBufferData(GL_ARRAY_BUFFER, sizeof(struct tile) * t, tile, GL_STREAM_DRAW);

		// Draw line loops:
		for (int i = 0; i < t; i++)
			glDrawArrays(GL_LINE_LOOP, i * 4, 4);
	}
}

static void
paint_center (void)
{
	// Actual center:
	double cx = viewport_get_center_x();
	double cy = viewport_get_center_y();

	glColor4f(1.0, 1.0, 1.0, 0.7);
	glBegin(GL_LINES);
		glVertex2d(cx - 0.5, cy);
		glVertex2d(cx + 0.5, cy);
		glVertex2d(cx, cy - 0.5);
		glVertex2d(cx, cy + 0.5);
	glEnd();
}

static void
paint_bbox (void)
{
	// Draw bounding box:
	double *bx, *by;
	viewport_get_bbox(&bx, &by);

	glColor4f(0.3, 0.3, 1.0, 0.5);
	glBegin(GL_LINE_LOOP);
		glVertex2d(bx[0], by[1]);
		glVertex2d(bx[1], by[1]);
		glVertex2d(bx[1], by[0]);
		glVertex2d(bx[0], by[0]);
	glEnd();
}

static void
paint (void)
{
	// Draw 1:1 to screen coordinates, origin bottom left:
	setup_viewport();

	// Paint background using solid program:
	program_solid_use(&((struct program_solid) {
		.matrix = mat_proj,
	}));

	paint_background(vao_bkgd);

	// Paint tiles using solid program:
	paint_tiles();

	// Paint background using frustum program:
	program_frustum_use(&((struct program_frustum) {
		.mat_proj = mat_proj,
		.mat_frustum = camera_mat_mvp(),
		.cx = viewport_get_center_x(),
		.cy = viewport_get_center_y(),
		.world_size = world_get_size(),
		.spherical = (viewport_mode_get() == VIEWPORT_MODE_SPHERICAL),
	}));

	paint_background(vao_frustum);

	// Reset program:
	program_none();

	paint_center();
	paint_bbox();

	glDisable(GL_BLEND);

	// Reset program:
	program_none();
}

static void
zoom (const unsigned int zoom)
{
	double size = world_get_size();

	// Make room within the world for one extra pixel at each side,
	// to keep the outlines on the far tiles within frame:
	double one_pixel_at_scale = (1 << zoom) / SIZE;
	size += one_pixel_at_scale;

	mat_ortho(mat_proj, 0, size, 0, size, 0, 1);

	// Background quad is array of counterclockwise vertices:
	//
	//   3--2
	//   |  |
	//   0--1
	//
	struct vertex bkgd[4] = {
		{ 0.0f, 0.0f, 0.3f, 0.3f, 0.3f, 0.5f },
		{ size, 0.0f, 0.3f, 0.3f, 0.3f, 0.5f },
		{ size, size, 0.3f, 0.3f, 0.3f, 0.5f },
		{ 0.0f, size, 0.3f, 0.3f, 0.3f, 0.5f },
	};

	// Bind vertex buffer object and upload vertices:
	glBindBuffer(GL_ARRAY_BUFFER, vbo_bkgd);
	glBufferData(GL_ARRAY_BUFFER, sizeof(bkgd), bkgd, GL_STREAM_DRAW);
}

static bool
init (void)
{
	// Generate vertex buffer objects:
	glGenBuffers(1, &vbo_bkgd);
	glGenBuffers(1, &vbo_tiles);

	// Generate vertex array objects:
	glGenVertexArrays(1, &vao_bkgd);
	glGenVertexArrays(1, &vao_tiles);
	glGenVertexArrays(1, &vao_frustum);

	// Bind buffer and vertex array:
	glBindBuffer(GL_ARRAY_BUFFER, vbo_bkgd);

	// Add pointer to 'vertex' attribute:
	glBindVertexArray(vao_bkgd);
	glEnableVertexAttribArray(program_solid_loc_vertex());
	glVertexAttribPointer(program_solid_loc_vertex(), 2, GL_FLOAT, GL_FALSE,
		sizeof(struct vertex),
		(void *)(&((struct vertex *)0)->x));

	// Add pointer to 'color' attribute:
	glEnableVertexAttribArray(program_solid_loc_color());
	glVertexAttribPointer(program_solid_loc_color(), 4, GL_FLOAT, GL_FALSE,
		sizeof(struct vertex),
		(void *)(&((struct vertex *)0)->r));

	// Add pointer to 'vertex' attribute:
	glBindVertexArray(vao_frustum);
	glEnableVertexAttribArray(program_frustum_loc_vertex());
	glVertexAttribPointer(program_frustum_loc_vertex(), 2, GL_FLOAT, GL_FALSE,
		sizeof(struct vertex),
		(void *)(&((struct vertex *)0)->x));

	// Bind buffer and vertex array:
	glBindBuffer(GL_ARRAY_BUFFER, vbo_tiles);
	glBindVertexArray(vao_tiles);

	// Add pointer to 'vertex' attribute:
	glEnableVertexAttribArray(program_solid_loc_vertex());
	glVertexAttribPointer(program_solid_loc_vertex(), 2, GL_FLOAT, GL_FALSE,
		sizeof(struct vertex),
		(void *)(&((struct vertex *)0)->x));

	// Add pointer to 'color' attribute:
	glEnableVertexAttribArray(program_solid_loc_color());
	glVertexAttribPointer(program_solid_loc_color(), 4, GL_FLOAT, GL_FALSE,
		sizeof(struct vertex),
		(void *)(&((struct vertex *)0)->r));

	zoom(0);

	return true;
}

static void
destroy (void)
{
	// Delete vertex array objects:
	glDeleteVertexArrays(1, &vao_bkgd);
	glDeleteVertexArrays(1, &vao_tiles);
	glDeleteVertexArrays(1, &vao_frustum);

	// Delete vertex buffers:
	glDeleteBuffers(1, &vbo_bkgd);
	glDeleteBuffers(1, &vbo_tiles);
}

struct layer *
layer_overview (void)
{
	static struct layer layer = {
		.init     = &init,
		.occludes = &occludes,
		.paint    = &paint,
		.zoom     = &zoom,
		.destroy  = &destroy,
	};

	return &layer;
}
