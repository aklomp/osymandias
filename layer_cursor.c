#include <stdbool.h>
#include <GL/gl.h>

#include "shaders.h"
#include "viewport.h"
#include "layers.h"

static bool
layer_cursor_full_occlusion (void)
{
	// The cursor, by design, never occludes:
	return false;
}

static void
layer_cursor_paint (void)
{
	float halfwd = (float)viewport_get_wd() / 2.0;
	float halfht = (float)viewport_get_ht() / 2.0;
	unsigned int center_x = viewport_get_center_x();
	unsigned int center_y = viewport_get_center_y();

	shader_use_cursor(halfwd, halfht);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glBegin(GL_QUADS);
		glVertex2f((float)center_x - 15.0, (float)center_y - 15.0);
		glVertex2f((float)center_x + 15.0, (float)center_y - 15.0);
		glVertex2f((float)center_x + 15.0, (float)center_y + 15.0);
		glVertex2f((float)center_x - 15.0, (float)center_y + 15.0);
	glEnd();
	glUseProgram(0);
	glDisable(GL_BLEND);
}

bool
layer_cursor_create (void)
{
	// Cursor should be topmost layer; set sufficiently large z-index:
	return layer_register(layer_create(layer_cursor_full_occlusion, layer_cursor_paint, NULL, NULL, NULL), 10000);
}
