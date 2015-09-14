#include <stdbool.h>
#include <GL/gl.h>

#include "inlinebin.h"
#include "programs.h"
#include "programs/cursor.h"
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

	// Draw 1:1 to screen coordinates, origin bottom left:
	viewport_gl_setup_screen();

	program_cursor_use(&((struct program_cursor) {
		.halfwd = halfwd,
		.halfht = halfht,
	}));

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glBegin(GL_QUADS);
		glVertex2f((float)halfwd - 15.0, (float)halfht - 15.0);
		glVertex2f((float)halfwd + 15.0, (float)halfht - 15.0);
		glVertex2f((float)halfwd + 15.0, (float)halfht + 15.0);
		glVertex2f((float)halfwd - 15.0, (float)halfht + 15.0);
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
