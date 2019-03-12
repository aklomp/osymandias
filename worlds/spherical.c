#include <stdbool.h>
#include <stdint.h>
#include <math.h>
#include <vec/vec.h>

#include "../matrix.h"
#include "../worlds.h"
#include "local.h"
#include "autoscroll.h"

// Various transformation matrices:
static struct {
	struct {
		float lat[16];
		float lon[16];
	} rotate;
	float translate[16];
	float scale[16];
	float model[16];
	struct {
		float model[16];
		bool model_fresh;
	} inverse;
} matrix;

static void
project (const struct world_state *state, union vec *vertex, const float lat, const float lon)
{
	(void)state;

	// Basic lat/lon projection on a unit sphere:
	vertex->x = cosf(lat) * sinf(lon);
	vertex->z = cosf(lat) * cosf(lon);
	vertex->y = sinf(lat);
	vertex->w = 1.0f;

	// Apply model matrix:
	mat_vec_multiply(vertex->elem.f, matrix.model, vertex->elem.f);
}

static void
update_matrix_model (void)
{
	mat_multiply(matrix.model, matrix.rotate.lat, matrix.rotate.lon);
	mat_multiply(matrix.model, matrix.translate, matrix.model);
	mat_multiply(matrix.model, matrix.scale, matrix.model);

	// This invalidates the inverse model matrix:
	matrix.inverse.model_fresh = false;
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

static const float *
matrix_model_inverse (void)
{
	// Lazy instantiation:
	if (!matrix.inverse.model_fresh) {
		mat_invert(matrix.inverse.model, matrix.model);
		matrix.inverse.model_fresh = true;
	}
	return matrix.inverse.model;
}

static bool
autoscroll_update (struct world_state *state, int64_t now)
{
	const struct mark   *free  = &state->autoscroll.free;
	const struct coords *speed = &state->autoscroll.speed;

	if (!state->autoscroll.active)
		return false;

	// Calculate next location from start, speed and time:
	float dt = now - free->time;
	float lat = free->coords.lat + speed->lat * dt;
	float lon = free->coords.lon + speed->lon * dt;

	world_moveto_latlon(lat, lon);
	return true;
}

static bool
timer_tick (struct world_state *state, int64_t usec)
{
	return autoscroll_update(state, usec);
}

const struct world *
world_spherical (void)
{
	static const struct world world = {
		.matrix			= matrix_model,
		.matrix_inverse		= matrix_model_inverse,
		.move			= move,
		.project		= project,
		.zoom			= zoom,
		.center_restrict_tile	= center_restrict_tile,
		.center_restrict_latlon	= center_restrict_latlon,
		.timer_tick		= timer_tick,
	};

	// Translate matrix: push the model back one unit radius:
	mat_translate(matrix.translate, 0.0f, 0.0f, -1.0f);

	return &world;
}
