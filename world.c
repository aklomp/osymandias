#include <stdbool.h>

// At zoom level 0, the world is 1 tile wide.
// At zoom level 1, it's two tiles.
// At zoom level 2, it's four tiles, etc.
#define ZOOM_SIZE(z)	(1U << (z))
#define ZOOM_MAX	18

static struct {
	unsigned int zoom;
	unsigned int size;
} world = {
	.zoom = 0,
	.size = ZOOM_SIZE(0),
};

unsigned int
world_get_size (void)
{
	return world.size;
}

unsigned int
world_get_zoom (void)
{
	return world.zoom;
}

bool
world_zoom_in (void)
{
	if (world.zoom == ZOOM_MAX)
		return false;

	world.size = ZOOM_SIZE(++world.zoom);
	return true;
}

bool
world_zoom_out (void)
{
	if (world.zoom == 0)
		return false;

	world.size = ZOOM_SIZE(--world.zoom);
	return true;
}
