#include <stdbool.h>

#define ZOOM_MIN	0
#define ZOOM_MAX	18
#define ZOOM_SIZE(z)	(1 << (z))

// Current zoomlevel:
static int world_zoom = ZOOM_MIN;

// Current world size in pixels:
static unsigned int world_size = ZOOM_SIZE(ZOOM_MIN);

// NB: the current world contracts and expands based on zoom level.
// The world is measured in tile units (which subdivide into 256 pixels).
// We previously measured directly in pixels, but it turned out that floats
// were not precise enough to perform pixel-perfect operations at the highest
// zoom levels.

unsigned int
world_get_size (void)
{
	// Return the current world size in pixels:
	return world_size;
}

unsigned int
world_get_size_at (const int zoomlevel)
{
	// Zoom level 0 = 1 tile
	// Zoom level 1 = 2 tiles
	// Zoom level 2 = 3 tiles
	// Zoom level n = 2^n tiles
	return ((unsigned int)1) << zoomlevel;
}

int
world_get_zoom (void)
{
	return world_zoom;
}

void
world_zoom_to (const int zoomlevel)
{
	world_zoom = zoomlevel;
	world_size = world_get_size_at(world_zoom);
}

bool
world_zoom_in (void)
{
	if (world_zoom == ZOOM_MAX) {
		return false;
	}
	world_zoom_to(world_zoom + 1);
	return true;
}

bool
world_zoom_out (void)
{
	if (world_zoom == ZOOM_MIN) {
		return false;
	}
	world_zoom_to(world_zoom - 1);
	return true;
}
