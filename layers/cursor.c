#include <stdbool.h>
#include <GL/gl.h>

#include "../matrix.h"
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
	float mat_proj[16];
	float mat_view[16];

	const float wd = viewport_get_wd();
	const float ht = viewport_get_ht();

	// Viewport is screen:
	glViewport(0, 0, wd, ht);

	// Projection matrix maps 1:1 to screen:
	mat_ortho(mat_proj, 0, wd, 0, ht, 0, 1);

	// Modelview matrix translates vertices to center screen:
	mat_translate(mat_view, wd / 2.0f, ht / 2.0f, 0.0f);

	glMatrixMode(GL_PROJECTION);
	glLoadMatrixf(mat_proj);

	glMatrixMode(GL_MODELVIEW);
	glLoadMatrixf(mat_view);

	glDisable(GL_BLEND);
	glDisable(GL_DEPTH_TEST);
	glDepthMask(GL_FALSE);

	program_cursor_use(&((struct program_cursor) {
		.halfwd = wd / 2.0f,
		.halfht = ht / 2.0f,
	}));

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glBegin(GL_QUADS);
		glVertex2f(-15.0f, -15.0f);
		glVertex2f( 15.0f, -15.0f);
		glVertex2f( 15.0f,  15.0f);
		glVertex2f(-15.0f,  15.0f);
	glEnd();

	program_none();
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
