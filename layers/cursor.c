#include <stdbool.h>
#include <GL/gl.h>

#include "../inlinebin.h"
#include "../layers.h"
#include "../programs.h"
#include "../programs/cursor.h"
#include "../viewport.h"

static bool
occludes (void)
{
	return false;
}

static void
paint (void)
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
	program_none();
	glDisable(GL_BLEND);
}

struct layer *
layer_cursor (void)
{
	static struct layer layer = {
		.occludes = &occludes,
		.paint    = &paint,
	};

	return &layer;
}
