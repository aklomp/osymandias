#include <stdbool.h>

#include <GL/gl.h>

#include "../matrix.h"
#include "../vector.h"
#include "../worlds.h"
#include "../viewport.h"
#include "../layers.h"

#define FOG_END	20.0

static void
paint (void)
{
	if (world_get() != WORLD_PLANAR)
		return;

	int world_size = world_get_size();
	const struct coords *center = world_get_center();
	const float *mat_model = world_get_matrix();

	// Draw to world coordinates:
	viewport_gl_setup_world();

	// Draw a giant quad to the current world size:
	float points[4][4] = {
		{ 0.0f,       0.0f,       0.0f, 1.0f },
		{ 0.0f,       world_size, 0.0f, 1.0f },
		{ world_size, world_size, 0.0f, 1.0f },
		{ world_size, 0.0f,       0.0f, 1.0f },
	};

	glColor3f(0.12, 0.12, 0.12);

	glBegin(GL_QUADS);
	for (int i = 0; i < 4; i++) {
		struct vector *p = (struct vector *)points[i];
		mat_vec_multiply(points[i], mat_model, points[i]);
		glVertex2f(p->x, p->y);
	}
	glEnd();

	// Setup fog:
	glEnable(GL_FOG);
	glFogfv(GL_FOG_COLOR, (float[]){ 0.12, 0.12, 0.12, 1.0 });
	glHint(GL_FOG_HINT, GL_DONT_CARE);
	glFogi(GL_FOG_MODE, GL_LINEAR);
	glFogf(GL_FOG_DENSITY, 1.0);
	glFogf(GL_FOG_START, 1.0);
	glFogf(GL_FOG_END, FOG_END);

	// Clip tile size to non-fog region:
	int l = center->tile.x - FOG_END - 5;
	int r = center->tile.x + FOG_END + 5;
	int t = world_size - center->tile.y + FOG_END + 5;
	int b = world_size - center->tile.y - FOG_END - 5;

	l = (l < 0) ? 0 : (l >= world_size) ? world_size : l;
	r = (r < 0) ? 0 : (r >= world_size) ? world_size : r;
	t = (t < 0) ? 0 : (t >= world_size) ? world_size : t;
	b = (b < 0) ? 0 : (b >= world_size) ? world_size : b;

	glColor3f(0.20, 0.20, 0.20);
	glBegin(GL_LINES);

	// Vertical grid lines:
	for (int x = l; x <= r; x++) {
		float line[2][4] = {
			{ x, t, 0.0f, 1.0f },
			{ x, b, 0.0f, 1.0f },
		};
		mat_vec_multiply(line[0], mat_model, line[0]);
		mat_vec_multiply(line[1], mat_model, line[1]);

		glVertex2d(line[0][0], line[0][1]);
		glVertex2d(line[1][0], line[1][1]);
	}

	// Horizontal grid lines:
	for (int y = b; y <= t; y++) {
		float line[2][4] = {
			{ l, y, 0.0f, 1.0f },
			{ r, y, 0.0f, 1.0f },
		};
		mat_vec_multiply(line[0], mat_model, line[0]);
		mat_vec_multiply(line[1], mat_model, line[1]);

		glVertex2d(line[0][0], line[0][1]);
		glVertex2d(line[1][0], line[1][1]);
	}

	glEnd();
	glDisable(GL_FOG);
}

const struct layer *
layer_blanktile (void)
{
	static struct layer layer = {
		.paint    = &paint,
	};

	return &layer;
}
