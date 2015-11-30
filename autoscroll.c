#include <stdbool.h>
#include <time.h>

struct mark {
	int x;
	int y;
	float time;
};

static struct mark down;	// mark when mouse button pressed
static struct mark hold;	// mark when last dragged under mouse button
static struct mark free;	// mark when mouse button released

// Scroll speed in pixels/sec:
static struct {
	float x;
	float y;
} speed = {
	.x = 0.0f,
	.y = 0.0f,
};

static int scrolled_x = 0;		// x offset traveled in current autoscroll
static int scrolled_y = 0;		// y offset traveled in current autoscroll

static bool autoscroll_on = false;		// boolean toggle

static float
now (void)
{
	struct timespec ts;

	if (clock_gettime(CLOCK_MONOTONIC, &ts) == 0)
		return ts.tv_sec + ts.tv_nsec / 1e9;

	return 0.0f;
}

static void
mark (struct mark *mark, const int x, const int y)
{
	mark->x = x;
	mark->y = y;
	mark->time = now();
}

void
autoscroll_measure_down (const int x, const int y)
{
	mark(&down, x, y);
}

void
autoscroll_measure_hold (const int x, const int y)
{
	mark(&hold, x, y);
}

void
autoscroll_may_start (const int x, const int y)
{
	mark(&free, x, y);

	// First calculate whether the user has "moved the hold" sufficiently
	// to start the autoscroll. If the mouse button is down, but the mouse
	// has not moved significantly, then don't autoscroll:
	int dx = free.x - hold.x;
	int dy = free.y - hold.y;
	float dt = free.time - hold.time;

	if (dt > 0.1 && dx > -12 && dx < 12 && dy > -12 && dy < 12) {
		autoscroll_on = false;
		return;
	}

	dx = free.x - down.x;
	dy = free.y - down.y;
	dt = free.time - down.time;

	// The 2.0 coefficient is "friction":
	speed.x = dx / dt / 2.0f;
	speed.y = dy / dt / 2.0f;

	scrolled_x = 0;
	scrolled_y = 0;
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
autoscroll_update (int *restrict new_dx, int *restrict new_dy)
{
	// Calculate new autoscroll offset to apply to viewport.
	// Returns true or false depending on whether to apply the result.
	if (!autoscroll_on)
		return false;

	// Compare where we are with where we should be:
	float dt = now() - free.time;
	float dx = speed.x * dt - (float)scrolled_x;
	float dy = speed.y * dt - (float)scrolled_y;

	// Viewport deals only in integer coordinates:
	*new_dx = (int)dx;
	*new_dy = (int)dy;

	scrolled_x += *new_dx;
	scrolled_y += *new_dy;

	return true;
}
