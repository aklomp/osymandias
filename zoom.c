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

static struct {
	int zoom;
} state;

static inline float
zoomlevel_to_distance (const int zoom)
{
	return exp2f(-zoom);
}

static void
apply_zoom (const float zoom)
{
	camera_set_distance(zoomlevel_to_distance(zoom));
}

void
zoom_out (void)
{
	if (state.zoom > ZOOM_MIN)
		apply_zoom(--state.zoom);
}

void
zoom_in (void)
{
	if (state.zoom < ZOOM_MAX)
		apply_zoom(++state.zoom);
}
