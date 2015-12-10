#include <stdbool.h>
#include <stdint.h>

#include "../worlds.h"
#include "local.h"

#define abs(x)	(((x) < 0) ? -(x) : (x))

static void
mark (const struct world_state *state, int64_t usec, struct mark *mark)
{
	mark->coords = state->center;
	mark->time = usec;
}

void
autoscroll_measure_down (struct world_state *state, int64_t usec)
{
	mark(state, usec, &state->autoscroll.down);
}

void
autoscroll_measure_hold (struct world_state *state, int64_t usec)
{
	mark(state, usec, &state->autoscroll.hold);
}

void
autoscroll_measure_free (struct world_state *state, int64_t usec)
{
	const struct mark *down = &state->autoscroll.down;
	const struct mark *hold = &state->autoscroll.hold;
	const struct mark *free = &state->autoscroll.free;

	mark(state, usec, &state->autoscroll.free);

	// Check if the user has "moved the hold" sufficiently to start the
	// autoscroll. Not every click on the map should cause movement. Only
	// "significant" drags count.

	float dx = free->coords.tile.x - hold->coords.tile.x;
	float dy = free->coords.tile.y - hold->coords.tile.y;
	float dt = (free->time - hold->time) / 1e6f;

	// If the mouse has been stationary for a little while, and the last
	// mouse movement was insignificant, don't start:
	if (dt > 0.1f && abs(dx) < 12.0f && abs(dy) < 12.0f)
		return;

	// Speed and direction of autoscroll is measured between "down" point
	// and "up" point:
	dx = free->coords.tile.x - down->coords.tile.x;
	dy = free->coords.tile.y - down->coords.tile.y;
	dt = (free->time - down->time) / 1e6f;

	// The 2.0 coefficient is "friction":
	state->autoscroll.speed.tile.x = dx / dt / 2.0f;
	state->autoscroll.speed.tile.y = dy / dt / 2.0f;

	state->autoscroll.active = true;
}

// Returns true if autoscroll was actually stopped:
bool
autoscroll_stop (struct world_state *state)
{
	bool on = state->autoscroll.active;
	state->autoscroll.active = false;
	return on;
}

bool
autoscroll_update (struct world_state *state, int64_t usec)
{
	const struct mark *free = &state->autoscroll.free;
	const struct coords *speed = &state->autoscroll.speed;

	// Calculate new autoscroll offset to apply to viewport.
	// Returns true or false depending on whether to apply the result.
	if (!state->autoscroll.active)
		return false;

	// Calculate present location from start and speed:
	float dt = (usec - free->time) / 1e6f;
	float x = free->coords.tile.x + speed->tile.x * dt;
	float y = free->coords.tile.y + speed->tile.y * dt;

	world_moveto_tile(x, y);
	return true;
}
