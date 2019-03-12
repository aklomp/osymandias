#include <stdbool.h>
#include <stdint.h>
#include <math.h>

#include "../matrix.h"
#include "../worlds.h"
#include "local.h"
#include "autoscroll.h"

// Transformation matrices:
static struct {
	float model[16];
	struct {
		float model[16];
		bool model_fresh;
	} inverse;
} matrix;

static inline void
latlon_to_world (const struct world_state *state, float *x, float *y, const float lat, const float lon)
{
	*x = state->size * (0.5f + lon / (2.0f * M_PI));
	*y = state->size * (0.5f + atanhf(sinf(lat)) / (2.0f * M_PI));
}

static void
project (const struct world_state *state, union vec *vertex, const float lat, const float lon)
{
	// Convert lat/lon to world coordinates:
	latlon_to_world(state, &vertex->x, &vertex->y, lat, lon);

	vertex->z = 0.0f;
	vertex->w = 1.0f;

	// Apply model matrix:
	mat_vec_multiply(vertex->elem.f, matrix.model, vertex->elem.f);
}

static void
update_matrix_model (const struct world_state *state)
{
	float x, y;

	// Get world coordinates of current position:
	x = state->center.tile.x;
	y = state->size - state->center.tile.y;

	// Translate world so that this position is at origin:
	mat_translate(matrix.model, -x, -y, 0.0f);

	// This invalidates the inverse model matrix:
	matrix.inverse.model_fresh = false;
}

static void
center_restrict_tile (struct world_state *state)
{
	// Clamp x and y coordinates to world size:
	clamp(state->center.tile.x, 0.0f, state->size);
	clamp(state->center.tile.y, 0.0f, state->size);
}

static void
center_restrict_latlon (struct world_state *state)
{
	clamp(state->center.lat, LAT_MIN, LAT_MAX);
	clamp(state->center.lon, LON_MIN, LON_MAX);
}

static void
move (const struct world_state *state)
{
	update_matrix_model(state);
}

static void
zoom (const struct world_state *state)
{
	update_matrix_model(state);
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
	float x = free->coords.tile.x + speed->tile.x * dt;
	float y = free->coords.tile.y + speed->tile.y * dt;

	world_moveto_tile(x, y);
	return true;
}

static bool
timer_tick (struct world_state *state, int64_t usec)
{
	return autoscroll_update(state, usec);
}

const struct world *
world_planar (void)
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

	return &world;
}
