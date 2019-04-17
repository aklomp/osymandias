#include <stdbool.h>
#include <stdint.h>

#include "../worlds.h"
#include "local.h"
#include "autoscroll.h"

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
	(void) state;
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
		.move			= move,
		.center_restrict_tile	= center_restrict_tile,
		.center_restrict_latlon	= center_restrict_latlon,
		.timer_tick		= timer_tick,
	};

	return &world;
}
