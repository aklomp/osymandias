#include <stdbool.h>
#include <time.h>
#include "worlds.h"

#define abs(x)	(((x) < 0) ? -(x) : (x))

struct mark {
	double x;
	double y;
	double time;
};

static struct mark down;	// mark when mouse button pressed
static struct mark hold;	// mark when last dragged under mouse button
static struct mark free;	// mark when mouse button released

// Scroll speed in pixels/sec:
static struct {
	double x;
	double y;
} speed = {
	.x = 0.0,
	.y = 0.0,
};

static bool autoscroll_on = false;		// boolean toggle

static double
now (void)
{
	struct timespec ts;

	if (clock_gettime(CLOCK_MONOTONIC, &ts) == 0)
		return ts.tv_sec + ts.tv_nsec / 1e9;

	return 0.0f;
}

static void
mark (struct mark *mark)
{
	const struct coords *center = world_get_center();

	// FIXME: use tile, not world, coordinates:
	mark->x = center->tile.x;
	mark->y = world_get_size() - center->tile.y;
	mark->time = now();
}

void
autoscroll_measure_down (void)
{
	mark(&down);
}

void
autoscroll_measure_hold (void)
{
	mark(&hold);
}

void
autoscroll_may_start (void)
{
	mark(&free);

	// First calculate whether the user has "moved the hold" sufficiently
	// to start the autoscroll. If the mouse button is down, but the mouse
	// has not moved significantly, then don't autoscroll:
	double dx = free.x - hold.x;
	double dy = free.y - hold.y;
	double dt = free.time - hold.time;

	if (dt > 0.1 && abs(dx) < 12 && abs(dy) < 12) {
		autoscroll_on = false;
		return;
	}

	dx = free.x - down.x;
	dy = free.y - down.y;
	dt = free.time - down.time;

	// The 2.0 coefficient is "friction":
	speed.x = dx / dt / 2.0f;
	speed.y = dy / dt / 2.0f;

	autoscroll_on = true;
}

// Returns true if autoscroll was actually stopped:
bool
autoscroll_stop (void)
{
	bool on = autoscroll_on;
	autoscroll_on = false;
	return on;
}

bool
autoscroll_is_on (void)
{
	return autoscroll_on;
}

bool
autoscroll_update (double *restrict x, double *restrict y)
{
	// Calculate new autoscroll offset to apply to viewport.
	// Returns true or false depending on whether to apply the result.
	if (!autoscroll_on)
		return false;

	// Compare where we are with where we should be:
	double dt = now() - free.time;
	*x = free.x + speed.x * dt;
	*y = free.y + speed.y * dt;

	return true;
}

// FIXME: these are temporary functions
void
autoscroll_zoom_in (void)
{
	down.x *= 2.0;
	down.y *= 2.0;
	hold.x *= 2.0;
	hold.y *= 2.0;
	free.x *= 2.0;
	free.y *= 2.0;
}

void
autoscroll_zoom_out (void)
{
	down.x /= 2.0;
	down.y /= 2.0;
	hold.x /= 2.0;
	hold.y /= 2.0;
	free.x /= 2.0;
	free.y /= 2.0;
}
