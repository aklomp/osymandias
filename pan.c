#include <math.h>

#include "camera.h"
#include "globe.h"
#include "pan.h"

// Maximal time in usec for a mousedown/mouseup event to register as a click:
#define CLICK_MAX_DURATION_USEC		200000

enum state {
	STATE_IDLE,
	STATE_DOWN,
	STATE_DOWN_NOCLICK,
	STATE_DRAG,
	STATE_PAN,
	STATE_MOVETO,
};

struct mark {
	double lat;
	double lon;
	struct {
		double lat;
		double lon;
	} speed;
	int64_t now;
};

static struct {
	struct mark down;
	struct mark prev;
	struct mark cur;
	struct {
		double lat;
		double lon;
	} speed;
	struct {
		int64_t start;
		int64_t last;
	} pan;
	struct {
		int64_t start;
		double lat;
		double lon;
	} moveto;
	enum state state;
} state;

static void
push_cursor (const struct globe *globe, const int64_t now)
{
	state.prev = state.cur;

	state.cur.lat = globe->lat;
	state.cur.lon = globe->lon;
	state.cur.now = now;
}

void
pan_on_button_down (const struct viewport_pos *pos, const int64_t now)
{
	float latf, lonf;
	bool stopped_pan = false;

	for (;;) {
		switch (state.state) {
		case STATE_IDLE:

			// Save the coordinates of the point under the mouse cursor at
			// the moment when the button goes down:
			if (viewport_unproject(pos, &latf, &lonf) == false)
				return;

			state.down.lat = (double) latf;
			state.down.lon = (double) lonf;
			state.down.now = now;
			state.state = stopped_pan ? STATE_DOWN_NOCLICK : STATE_DOWN;
			break;

		case STATE_DOWN:
		case STATE_DOWN_NOCLICK:

			// Save the current cursor position:
			push_cursor(globe_get(), now);

			// Set initial velocity:
			state.cur.speed.lat = 0.0;
			state.cur.speed.lon = 0.0;

			// Reset integrators:
			state.speed.lon = 0.0;
			state.speed.lat = 0.0;
			return;

		case STATE_MOVETO:
		case STATE_PAN:

			// The mousedown stopped the pan, and should not be
			// interpreted as the start of a click:
			stopped_pan = true;
			state.state = STATE_IDLE;
			break;

		case STATE_DRAG:

			// Can't really happen:
			state.state = STATE_IDLE;
			break;
		}
	}
}

static void
sum_relative_speeds (void)
{
	const struct camera *cam = camera_get();
	const double dt = state.cur.now - state.prev.now;

	// v = dx/dt:
	state.cur.speed.lat = (state.cur.lat - state.prev.lat) / cam->distance / dt;
	state.cur.speed.lon = (state.cur.lon - state.prev.lon) / cam->distance / dt;

	// dv = d1 - d0:
	state.speed.lat += (state.cur.speed.lat - state.prev.speed.lat);
	state.speed.lon += (state.cur.speed.lon - state.prev.speed.lon);
}

static bool
move_cursor (const struct globe *globe, const struct viewport_pos *pos)
{
	float latf, lonf;
	double lat, lon;

	// Get the coordinates of the point currently under the mouse cursor:
	if (viewport_unproject(pos, &latf, &lonf) == false)
		return false;

	// Move the center cursor such that the point under the mouse cursor
	// will be the same point that was under the mouse cursor when the
	// button first went down. This appears to the user as a drag:
	lat = globe->lat + (state.down.lat - (double) latf);
	lon = globe->lon + (state.down.lon - (double) lonf);

	globe_moveto(lat, lon);
	return true;
}

bool
pan_on_button_move (const struct viewport_pos *pos, const int64_t now)
{
	for (;;) {
		switch (state.state) {
		case STATE_DOWN:
		case STATE_DOWN_NOCLICK:

			// The mouse-down now becomes a drag:
			state.state = STATE_DRAG;
			break;

		case STATE_DRAG: {
			const struct globe *globe = globe_get();

			if (move_cursor(globe, pos) == false) {
				state.state = STATE_IDLE;
				return false;
			}

			push_cursor(globe, now);
			sum_relative_speeds();
			return true;
		}

		default:

			// Other states shouldn't be reachable, they should go
			// through the down state first:
			return false;
		}
	}
}

void
pan_on_button_up (const struct viewport_pos *pos, const int64_t now)
{
	(void) pos;

	switch (state.state) {
	case STATE_DOWN:

		// Mouse went down, then up: it's a click:
		if ((now - state.down.now) > CLICK_MAX_DURATION_USEC) {
			state.state = STATE_IDLE;
			break;
		}

		state.state        = STATE_MOVETO;
		state.moveto.lat   = state.down.lat;
		state.moveto.lon   = state.down.lon;
		state.moveto.start = now;
		break;

	case STATE_DOWN_NOCLICK:
		state.state = STATE_IDLE;
		break;

	case STATE_DRAG:

		// Only turn a drag into a pan if the time since the last mouse
		// movement is sufficiently small (it's actually dragging):
		if ((now - state.cur.now) > 1e5) {
			state.state = STATE_IDLE;
			break;
		}

		state.state = STATE_PAN;
		state.pan.start = state.pan.last = now;
		break;

	default:
		break;
	}
}

bool
pan_on_tick (const int64_t now)
{
	switch (state.state) {
	case STATE_PAN: {
		const struct camera *cam = camera_get();
		const struct globe *globe = globe_get();

		// Easing factor: start fast, end slow:
		const double ease = sqrt((now - state.pan.start) / 1e5);

		const double dt  = now - state.pan.last;
		const double lat = globe->lat + state.speed.lat * dt * cam->distance / ease;
		const double lon = globe->lon + state.speed.lon * dt * cam->distance / ease;

		globe_moveto(lat, lon);

		state.pan.last = now;
		return true;
	}

	case STATE_MOVETO: {

		// Time since start in seconds:
		const double dt = (now - state.moveto.start) / 1e6;

		// Leave this state after a set time:
		if (dt >= 0.5) {
			globe_moveto(state.moveto.lat, state.moveto.lon);
			state.state = STATE_IDLE;
			return true;
		}

		const struct globe *globe = globe_get();

		// Distance left to cover:
		double dlat = state.moveto.lat - globe->lat;
		double dlon = state.moveto.lon - globe->lon;

		// Ensure longitude takes the short way round:
		if (dlon > M_PI)
			dlon -= 2.0 * M_PI;

		if (dlon < -M_PI)
			dlon += 2.0 * M_PI;

		// The speed is 1:1 proportional to remaining distance:
		const double lat = globe->lat + dlat * dt;
		const double lon = globe->lon + dlon * dt;

		globe_moveto(lat, lon);
		return true;
	}

	default:
		return false;
	}
}
