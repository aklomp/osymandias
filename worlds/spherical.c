#include <stdbool.h>
#include <stdint.h>
#include <math.h>

#include "../matrix.h"
#include "../vector.h"
#include "../worlds.h"
#include "local.h"

// Various transformation matrices:
static struct {
	struct {
		float lat[16];
		float lon[16];
	} rotate;
	float translate[16];
	float scale[16];
	float model[16];
} matrix;

static void
project (const struct world_state *state, float *vertex, float *normal, const float lat, const float lon)
{
	(void)state;

	struct vector *v = (struct vector *)vertex;
	struct vector *n = (struct vector *)normal;

	// Basic lat/lon projection on a unit sphere:
	v->x = cosf(lat) * sinf(lon);
	v->z = cosf(lat) * cosf(lon);
	v->y = sinf(lat);
	v->w = 1.0f;

	// Normal is identical to position, but with zero w:
	n->x = v->x;
	n->y = v->y;
	n->z = v->z;
	n->w = 0.0f;

	// Apply model matrix:
	mat_vec_multiply(vertex, matrix.model, vertex);
	mat_vec_multiply(normal, matrix.model, normal);
}

static void
update_matrix_model (void)
{
	mat_multiply(matrix.model, matrix.rotate.lat, matrix.rotate.lon);
	mat_multiply(matrix.model, matrix.translate, matrix.model);
	mat_multiply(matrix.model, matrix.scale, matrix.model);
}

static void
center_restrict_tile (struct world_state *state)
{
	// x coordinates are wrapped around:
	wrap(state->center.tile.x, 0.0f, state->size);

	// y coordinates are clamped:
	clamp(state->center.tile.y, 0.0f, state->size);
}

static void
center_restrict_latlon (struct world_state *state)
{
	clamp(state->center.lat, LAT_MIN, LAT_MAX);
}

static void
move (const struct world_state *state)
{
	// Rotate longitude into view over y axis:
	mat_rotate(matrix.rotate.lon, 0.0f, 1.0f, 0.0f, state->center.lon);

	// Rotate latitude into view over x axis:
	mat_rotate(matrix.rotate.lat, -1.0f, 0.0f, 0.0f, state->center.lat);

	// Update model matrix:
	update_matrix_model();
}

static void
zoom (const struct world_state *state)
{
	float radius = state->size / M_PI;

	// Translate matrix: push the model back one unit radius:
	mat_translate(matrix.translate, 0.0f, 0.0f, -1.0f);

	// Scale matrix: scale the model (a unit sphere) to the world radius:
	mat_scale(matrix.scale, radius, radius, radius);

	// Update model matrix:
	update_matrix_model();
}

static const float *
matrix_model (void)
{
	return matrix.model;
}

static bool
timer_tick (struct world_state *state, int64_t usec)
{
	return false;
}

const struct world *
world_spherical (void)
{
	static const struct world world = {
		.matrix			= matrix_model,
		.move			= move,
		.project		= project,
		.zoom			= zoom,
		.center_restrict_tile	= center_restrict_tile,
		.center_restrict_latlon	= center_restrict_latlon,
		.timer_tick		= timer_tick,
	};

	return &world;
}
