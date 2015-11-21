#include <stdbool.h>
#include <stdint.h>
#include <time.h>
#include "worlds.h"

#define abs(x)	(((x) < 0) ? -(x) : (x))

struct mark {
	double x;
	double y;
	double t;
};

static struct mark down;	// mouse button was first pressed here
static struct mark hold;	// last known mouse position under drag
static struct mark free;	// mouse was released here

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
	mark->t = now();
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
autoscroll_measure_free (void)
{
	mark(&free);

	// Check if the user has "moved the hold" sufficiently to start the
	// autoscroll. Not every click on the map should cause movement. Only
	// "significant" drags count.

	double dx = free.x - hold.x;
	double dy = free.y - hold.y;
	double dt = free.t - hold.t;

	// If the mouse has been stationary for a little while, and the last
	// mouse movement was insignificant, don't start:
	if (dt > 0.1 && abs(dx) < 12.0 && abs(dy) < 12.0)
		return;

	// Speed and direction of autoscroll is measured between "down" point
	// and "up" point:
	dx = free.x - down.x;
	dy = free.y - down.y;
	dt = free.t - down.t;

	// The 2.0 coefficient is "friction":
	speed.x = dx / dt / 2.0;
	speed.y = dy / dt / 2.0;

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
	double dt = now() - free.t;
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
