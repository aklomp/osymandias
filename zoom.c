#include <math.h>

#include "camera.h"
#include "zoom.h"

// Min and max zoom levels. These zoom levels have no direct relationship to
// tile zoom levels, although there is an indirect relationship. The zoom
// levels in this file determine the distance from the camera to the cursor as
// a power of two. As this distance decreases, tiles at higher zoom levels will
// be selected for display. One zoom level step should correspond to one shift
// in tile zoom level.
#define ZOOM_MIN	-4
#define ZOOM_MAX	16

enum state {
	STATE_IDLE,
	STATE_ZOOMING,
};

static struct {
	int zoom;
	struct {
		double  goal;
		int64_t start;
	} zooming;
	enum state state;
} state;

static inline double
zoomlevel_to_distance (const int zoom)
{
	return exp2(-zoom);
}

static void
apply_zoom (const double zoom)
{
	camera_set_distance(zoomlevel_to_distance(zoom));
}

bool
zoom_on_tick (const int64_t now)
{
	if (state.state != STATE_ZOOMING)
		return false;

	// Time since start in seconds:
	const double dt = (now - state.zooming.start) / 1e6;

	// Leave this state after a set time:
	if (dt >= 0.5) {
		apply_zoom(state.zoom);
		state.state = STATE_IDLE;
		return true;
	}

	const struct camera *cam = camera_get();

	// Distance left to cover:
	const double dx = state.zooming.goal - cam->distance;

	// The speed is 1:1 proportional to remaining distance:
	camera_set_distance(cam->distance + dx * dt);
	return true;
}

void
zoom_out (const int64_t now)
{
	if (state.zoom == ZOOM_MIN)
		return;

	state.state         = STATE_ZOOMING;
	state.zooming.start = now;
	state.zooming.goal  = zoomlevel_to_distance(--state.zoom);
}

void
zoom_in (const int64_t now)
{
	if (state.zoom == ZOOM_MAX)
		return;

	state.state         = STATE_ZOOMING;
	state.zooming.start = now;
	state.zooming.goal  = zoomlevel_to_distance(++state.zoom);
}
