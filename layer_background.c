#include <stdbool.h>
#include <GL/gl.h>

#include "shaders.h"
#include "world.h"
#include "viewport.h"
#include "layers.h"

// The background layer is the diagonal pinstripe pattern that is shown in the
// out-of-bounds area of the viewport.

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
	unsigned int screen_wd = viewport_get_wd();
	unsigned int screen_ht = viewport_get_ht();
	unsigned int center_x = viewport_get_center_x();
	unsigned int center_y = viewport_get_center_y();
	float halfwd = (float)screen_wd / 2.0 + 1.0;
	float halfht = (float)screen_ht / 2.0 + 1.0;

	shader_use_bkgd(world_get_size(), (center_x - screen_wd / 2) & 0xFF, (center_y - screen_ht / 2) & 0xFF);
	glBegin(GL_QUADS);
		glVertex2f(center_x - halfwd, center_y - halfht);
		glVertex2f(center_x + halfwd, center_y - halfht);
		glVertex2f(center_x + halfwd, center_y + halfht);
		glVertex2f(center_x - halfwd, center_y + halfht);
	glEnd();
	glUseProgram(0);
}

bool
layer_background_create (void)
{
	return layer_register(layer_create(layer_background_full_occlusion, layer_background_paint, NULL, NULL, NULL), 0);
}
