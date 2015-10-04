#include <stdbool.h>
#include <time.h>

static int down_x;			// x value at start of measurement, in window coordinates
static int down_y;			// y value at start of measurement, in window coordinates
static struct timespec down_t;		// time at start of measurement

static int hold_x;
static int hold_y;
static struct timespec hold_t;

static float base_dx = 0.0;		// the main dx value
static float base_dy = 0.0;		// the main dy value
static int scrolled_x = 0;		// x offset traveled in current autoscroll
static int scrolled_y = 0;		// y offset traveled in current autoscroll
static struct timespec base_t;		// start time of current autoscroll movement

static bool autoscroll_on = 0;		// boolean toggle

static float
timespec_diff (struct timespec *restrict earlier, struct timespec *restrict later)
{
	// Time difference in floating-point seconds:
	return ((float)later->tv_sec + ((float)later->tv_nsec) / 1e9)
	     - ((float)earlier->tv_sec + ((float)earlier->tv_nsec) / 1e9);
}

void
autoscroll_measure_down (const int x, const int y)
{
	// Note current position and time. This is converted to a dx, dy and dt
	// when we call autoscroll_kickoff().

	if (clock_gettime(CLOCK_MONOTONIC, &down_t) != 0) {
		return;
	}
	down_x = x;
	down_y = y;
}

void
autoscroll_measure_hold (const int x, const int y)
{
	if (clock_gettime(CLOCK_MONOTONIC, &hold_t) != 0) {
		return;
	}
	hold_x = x;
	hold_y = y;
}

void
autoscroll_may_start (const int x, const int y)
{
	if (clock_gettime(CLOCK_MONOTONIC, &base_t) != 0) {
		autoscroll_on = 0;
		return;
	}
	// First calculate whether the user has "moved the hold" sufficiently
	// to start the autoscroll. If the mouse button is down, but the mouse
	// has not moved significantly, then don't autoscroll:
	float dt = timespec_diff(&hold_t, &base_t);
	int dx = x - hold_x;
	int dy = y - hold_y;

	if (dt > 0.1 && dx > -12 && dx < 12 && dy > -12 && dy < 12) {
		autoscroll_on = 0;
		return;
	}
	dt = timespec_diff(&down_t, &base_t);

	// The 2.0 coefficient is "friction":
	base_dx = (float)(x - down_x) / 2.0 / dt;
	base_dy = (float)(y - down_y) / 2.0 / dt;

	scrolled_x = 0;
	scrolled_y = 0;
	autoscroll_on = 1;
}

void
autoscroll_stop (void)
{
	autoscroll_on = 0;
}

bool
autoscroll_is_on (void)
{
	return autoscroll_on;
}

bool
autoscroll_update (int *restrict new_dx, int *restrict new_dy)
{
	struct timespec now;

	// Calculate new autoscroll offset to apply to viewport.
	// Returns 0 or 1 depending on whether to apply the result.
	if (autoscroll_on == false || clock_gettime(CLOCK_MONOTONIC, &now) != 0) {
		return false;
	}
	float dt = timespec_diff(&base_t, &now);

	// Compare where we are with where we should be:
	float dx = base_dx * dt - (float)scrolled_x;
	float dy = base_dy * dt - (float)scrolled_y;

	// Viewport deals only in integer coordinates:
	*new_dx = (int)dx;
	*new_dy = (int)dy;

	scrolled_x += *new_dx;
	scrolled_y += *new_dy;

	return true;
}
