#include <stdbool.h>
#include <GL/gl.h>

#include "world.h"
#include "viewport.h"
#include "layers.h"
#include "tilepicker.h"

#define OVERVIEW_WD	256.0
#define OVERVIEW_MARGIN	10.0

static bool
layer_overview_full_occlusion (void)
{
	// The OVERVIEW, by design, never occludes:
	return false;
}

static void
layer_overview_paint (void)
{
	double world_size = world_get_size();

	// Draw 1:1 to screen coordinates, origin bottom left:
	viewport_gl_setup_overview(OVERVIEW_WD, OVERVIEW_MARGIN);

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	// World plane background:
	glColor4f(0.3, 0.3, 0.3, 0.5);
	glBegin(GL_QUADS);
		glVertex2d(0, 0);
		glVertex2d(0, world_size);
		glVertex2d(world_size, world_size);
		glVertex2d(world_size, 0);
	glEnd();

	// Draw tiles from tile picker:
	int x, y, tile_wd, tile_ht, zoom;
	float p[4][3];

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

bool
layer_overview_create (void)
{
	// Cursor should be topmost layer; set sufficiently large z-index:
	return layer_register(layer_create(layer_overview_full_occlusion, layer_overview_paint, NULL, NULL, NULL), 500);
}
