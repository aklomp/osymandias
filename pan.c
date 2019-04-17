#include <stdbool.h>

#include "globe.h"
#include "pan.h"

enum state {
	STATE_IDLE,
	STATE_DRAG,
};

struct mark {
	float lat;
	float lon;
};

static struct {
	struct mark down;
	enum state state;
} state;

void
pan_on_button_down (const struct viewport_pos *pos)
{
	switch (state.state)
	{
	case STATE_IDLE:

		// Save the coordinates of the point under the mouse cursor at
		// the moment when the button goes down:
		if (viewport_unproject(pos, &state.down.lat, &state.down.lon)) {
			state.state = STATE_DRAG;
		}
		break;

	default:
		break;
	}
}

static bool
move_cursor (const struct globe *globe, const struct viewport_pos *pos)
{
	float lat, lon;

	// Get the coordinates of the point currently under the mouse cursor:
	if (viewport_unproject(pos, &lat, &lon) == false)
		return false;

	// Move the center cursor such that the point under the mouse cursor
	// will be the same point that was under the mouse cursor when the
	// button first went down. This appears to the user as a drag:
	lat = globe->lat + (state.down.lat - lat);
	lon = globe->lon + (state.down.lon - lon);

	globe_moveto(lat, lon);
	return true;
}

void
pan_on_button_move (const struct viewport_pos *pos)
{
	switch (state.state)
	{
	case STATE_DRAG:
		move_cursor(globe_get(), pos);
		break;

	default:
		break;
	}
}
