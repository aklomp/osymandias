#include <stdbool.h>
#include <GL/gl.h>

#include "../world.h"
#include "../viewport.h"
#include "../layers.h"
#include "../tilepicker.h"

#define SIZE	256.0f
#define MARGIN	 10.0f

static bool
occludes (void)
{
	// The overview never occludes:
	return false;
}

static inline void
setup_viewport (int world_zoom, double world_size)
{
	// Make room within the world for one extra pixel at each side,
	// to keep the outlines on the far tiles within frame:
	double one_pixel_at_scale = (1 << world_zoom) / SIZE;
	world_size += one_pixel_at_scale;

	int xpos = viewport_get_wd() - MARGIN - SIZE;
	int ypos = viewport_get_ht() - MARGIN - SIZE;

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glLineWidth(1.0);
	glOrtho(0, world_size, 0, world_size, 0, 1);
	glViewport(xpos, ypos, SIZE, SIZE);

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();

	glDisable(GL_DEPTH_TEST);
	glDepthMask(GL_FALSE);

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}

static void
paint (void)
{
	int    world_zoom = world_get_zoom();
	double world_size = world_get_size();

	// Draw 1:1 to screen coordinates, origin bottom left:
	setup_viewport(world_zoom, world_size);

	// World plane background:
	glColor4f(0.3, 0.3, 0.3, 0.5);
	glBegin(GL_QUADS);
		glVertex2d(0, 0);
		glVertex2d(0, world_size);
		glVertex2d(world_size, world_size);
		glVertex2d(world_size, 0);
	glEnd();

	// Draw tiles from tile picker:
	int zoom;
	float x, y, tile_wd, tile_ht, p[4][3];

	for (int iter = tilepicker_first(&x, &y, &tile_wd, &tile_ht, &zoom, p); iter; iter = tilepicker_next(&x, &y, &tile_wd, &tile_ht, &zoom, p)) {
		float zoomcolors[6][3] = {
			{ 1.0, 0.0, 0.0 },
			{ 0.0, 1.0, 0.0 },
			{ 0.0, 0.0, 1.0 },
			{ 0.5, 0.5, 0.0 },
			{ 0.0, 0.5, 0.5 },
			{ 0.5, 0.0, 0.5 }
		};
		glColor4f(zoomcolors[zoom % 3][0], zoomcolors[zoom % 3][1], zoomcolors[zoom % 3][2], 0.5);
		glBegin(GL_QUADS);
			glVertex2f(x, world_size - y);
			glVertex2f(x + tile_wd, world_size - y);
			glVertex2f(x + tile_wd, world_size - y - tile_ht);
			glVertex2f(x, world_size - y - tile_ht);
		glEnd();
		glColor4f(1.0, 1.0, 1.0, 0.5);
		glBegin(GL_LINE_LOOP);
			glVertex2d(x, world_size - y);
			glVertex2d(x + tile_wd, world_size - y);
			glVertex2d(x + tile_wd, world_size - y - tile_ht);
			glVertex2d(x, world_size - y - tile_ht);
		glEnd();
	}
	// Draw frustum (view region):
	double *wx, *wy;
	viewport_get_frustum(&wx, &wy);

	glColor4f(1.0, 0.3, 0.3, 0.5);
	glBegin(GL_QUADS);
	for (int i = 0; i < 4; i++) {
		glVertex2d(wx[i], wy[i]);
	}
	glEnd();

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

	glDisable(GL_BLEND);
}

struct layer *
layer_overview (void)
{
	static struct layer layer = {
		.occludes = &occludes,
		.paint    = &paint,
	};

	return &layer;
}
