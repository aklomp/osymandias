#include <stdbool.h>

#define ZOOM_MAX	18
#define ZOOM_SIZE(z)	(1U << (z))

static struct {
	unsigned int zoom;
	unsigned int size;
} world = {
	.zoom = 0,
	.size = ZOOM_SIZE(0),
};

// NB: the current world contracts and expands based on zoom level.
// The world is measured in tile units (which subdivide into 256 pixels).
// We previously measured directly in pixels, but it turned out that floats
// were not precise enough to perform pixel-perfect operations at the highest
// zoom levels.

unsigned int
world_get_size (void)
{
	// Return the current world size in pixels:
	return world.size;
}

int
world_get_zoom (void)
{
	return world.zoom;
}

bool
world_zoom_in (void)
{
	if (world.zoom == ZOOM_MAX)
		return false;

	world.zoom++;
	world.size = 1U << world.zoom;
	return true;
}

bool
world_zoom_out (void)
{
	if (world.zoom == 0)
		return false;

	world.zoom--;
	world.size = 1U << world.zoom;
	return true;
}
