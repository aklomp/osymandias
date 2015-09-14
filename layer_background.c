#include <stdbool.h>
#include <GL/gl.h>

#include "inlinebin.h"
#include "programs.h"
#include "viewport.h"
#include "layers.h"

// The background layer is the diagonal pinstripe pattern that is shown in the
// out-of-bounds area of the viewport.

#define PINSTRIPE_PITCH	5

static bool
layer_background_full_occlusion (void)
{
	// The background always fully occludes:
	return true;
}

static void
layer_background_paint (void)
{
	// Nothing to do if the viewport is completely within world bounds:
	if (viewport_within_world_bounds()) {
		return;
	}
	int screen_wd = viewport_get_wd();
	int screen_ht = viewport_get_ht();

	// Draw 1:1 to screen coordinates, origin bottom left:
	viewport_gl_setup_screen();

	// Solid background fill:
	glClearColor(0.12, 0.12, 0.12, 1.0);
	glClear(GL_COLOR_BUFFER_BIT);

	// Pinstripe color:
	glColor3f(0.20, 0.20, 0.20);
	glBegin(GL_LINES);

	// Pinstripes along left edge:
	for (int y = 0; y < screen_ht; y += PINSTRIPE_PITCH) {
		glVertex2f(0.0, y);
		glVertex2f(screen_wd, y + screen_wd);
	}
	// Pinstripes along bottom edge:
	for (int x = PINSTRIPE_PITCH; x < screen_wd; x += PINSTRIPE_PITCH) {
		glVertex2f(x, 0.0);
		glVertex2f(x + screen_ht, screen_ht);
	}
	glEnd();
}

bool
layer_background_create (void)
{
	return layer_register(layer_create(layer_background_full_occlusion, layer_background_paint, NULL, NULL, NULL), 0);
}
