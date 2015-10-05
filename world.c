#include <stdbool.h>

#define ZOOM_MIN	0
#define ZOOM_MAX	18
#define ZOOM_SIZE(z)	(1 << (z))

// Current zoomlevel:
static int world_zoom = ZOOM_MIN;

// Current world size in tiles:
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

static unsigned int
size_at (const int zoomlevel)
{
	// Zoom level 0 = 1 tile
	// Zoom level 1 = 2 tiles
	// Zoom level 2 = 4 tiles
	// Zoom level n = 2^n tiles
	return 1U << zoomlevel;
}

int
world_get_zoom (void)
{
	return world_zoom;
}

static bool
zoom_to (const int zoomlevel)
{
	if (zoomlevel < ZOOM_MIN)
		return false;

	if (zoomlevel > ZOOM_MAX)
		return false;

	world_zoom = zoomlevel;
	world_size = size_at(world_zoom);
	return true;
}

bool
world_zoom_in (void)
{
	return zoom_to(world_zoom + 1);
}

bool
world_zoom_out (void)
{
	return zoom_to(world_zoom - 1);
}
