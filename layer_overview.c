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
	double wd = (double)viewport_get_wd();
	double ht = (double)viewport_get_ht();
	double world_size = world_get_size();

	// Draw 1:1 to screen coordinates, origin bottom left:
	viewport_gl_setup_screen();

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	// World plane background:
	glColor4f(0.3, 0.3, 0.3, 0.5);
	glBegin(GL_QUADS);
		glVertex2d(wd - OVERVIEW_MARGIN - OVERVIEW_WD, ht - OVERVIEW_MARGIN - OVERVIEW_WD);
		glVertex2d(wd - OVERVIEW_MARGIN,          ht - OVERVIEW_MARGIN - OVERVIEW_WD);
		glVertex2d(wd - OVERVIEW_MARGIN,          ht - OVERVIEW_MARGIN);
		glVertex2d(wd - OVERVIEW_MARGIN - OVERVIEW_WD, ht - OVERVIEW_MARGIN);
	glEnd();

	// Draw tiles from tile picker:
	int x, y, tile_wd, tile_ht, zoom;

	for (int iter = tilepicker_first(&x, &y, &tile_wd, &tile_ht, &zoom); iter; iter = tilepicker_next(&x, &y, &tile_wd, &tile_ht, &zoom)) {
		float zoomcolors[6][3] = {
			{ 1.0, 0.0, 0.0 },
			{ 0.0, 1.0, 0.0 },
			{ 0.0, 0.0, 1.0 },
			{ 0.5, 0.5, 0.0 },
			{ 0.0, 0.5, 0.5 },
			{ 0.5, 0.0, 0.5 }
		};
		glColor4f(zoomcolors[zoom % 3][0], zoomcolors[zoom % 3][1], zoomcolors[zoom % 3][2], 0.5);
		float xp[2] = {
			wd - OVERVIEW_MARGIN - OVERVIEW_WD + OVERVIEW_WD * (x / world_size),
			wd - OVERVIEW_MARGIN - OVERVIEW_WD + OVERVIEW_WD * ((x + tile_wd) / world_size)
		};
		float yp[2] = {
			ht - OVERVIEW_MARGIN - OVERVIEW_WD + OVERVIEW_WD * ((world_size - y)  / world_size),
			ht - OVERVIEW_MARGIN - OVERVIEW_WD + OVERVIEW_WD * ((world_size - y - tile_ht) / world_size)
		};
		glBegin(GL_QUADS);
			glVertex2f(xp[0], yp[0]);
			glVertex2f(xp[0], yp[1]);
			glVertex2f(xp[1], yp[1]);
			glVertex2f(xp[1], yp[0]);
		glEnd();
		glColor4f(1.0, 1.0, 1.0, 0.5);
		glBegin(GL_LINE_LOOP);
			glVertex2d(xp[0], yp[0]);
			glVertex2d(xp[0], yp[1]);
			glVertex2d(xp[1], yp[1]);
			glVertex2d(xp[1], yp[0]);
		glEnd();
	}

	// Draw frustum (view region):
	double *wx, *wy;
	viewport_get_frustum(&wx, &wy);

	glColor4f(1.0, 0.3, 0.3, 0.5);
	glBegin(GL_QUADS);
	for (int i = 0; i < 4; i++) {
		double a = wd - OVERVIEW_MARGIN - OVERVIEW_WD + OVERVIEW_WD * (wx[i] / world_size);
		double b = ht - OVERVIEW_MARGIN - OVERVIEW_WD + OVERVIEW_WD * (wy[i] / world_size);
		glVertex2d(a, b);
	}
	glEnd();

	// Actual center:
	double cx = viewport_get_center_x();
	double cy = viewport_get_center_y();

	cx = wd - OVERVIEW_MARGIN - OVERVIEW_WD + OVERVIEW_WD * (cx / world_size);
	cy = ht - OVERVIEW_MARGIN - OVERVIEW_WD + OVERVIEW_WD * (cy / world_size);

	glColor4f(1.0, 1.0, 1.0, 0.7);
	glBegin(GL_LINES);
		glVertex2d(cx - 1, cy);
		glVertex2d(cx + 1, cy);
		glVertex2d(cx, cy - 1);
		glVertex2d(cx, cy + 1);
	glEnd();

	// Draw bounding box:
	double *bx, *by;
	viewport_get_bbox(&bx, &by);

	double x0 = wd - OVERVIEW_MARGIN - OVERVIEW_WD + OVERVIEW_WD * (bx[0] / world_size);
	double x1 = wd - OVERVIEW_MARGIN - OVERVIEW_WD + OVERVIEW_WD * (bx[1] / world_size);
	double y0 = ht - OVERVIEW_MARGIN - OVERVIEW_WD + OVERVIEW_WD * (by[0] / world_size);
	double y1 = ht - OVERVIEW_MARGIN - OVERVIEW_WD + OVERVIEW_WD * (by[1] / world_size);

	glColor4f(0.3, 0.3, 1.0, 0.5);
	glBegin(GL_LINE_LOOP);
		glVertex2d(x0, y1);
		glVertex2d(x1, y1);
		glVertex2d(x1, y0);
		glVertex2d(x0, y0);
	glEnd();

	glDisable(GL_BLEND);
}

bool
layer_overview_create (void)
{
	// Cursor should be topmost layer; set sufficiently large z-index:
	return layer_register(layer_create(layer_overview_full_occlusion, layer_overview_paint, NULL, NULL, NULL), 500);
}
