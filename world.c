#include <stdbool.h>

// At zoom level 0, the world is 1 tile wide.
// At zoom level 1, it's two tiles.
// At zoom level 2, it's four tiles, etc.
#define ZOOM_SIZE(z)	(1U << (z))
#define ZOOM_MAX	18

static struct {
	unsigned int zoom;
	unsigned int size;
} state;

unsigned int
world_get_size (void)
{
	return state.size;
}

unsigned int
world_get_zoom (void)
{
	return state.zoom;
}

bool
world_zoom_in (void)
{
	if (state.zoom == ZOOM_MAX)
		return false;

	state.size = ZOOM_SIZE(++state.zoom);
	return true;
}

bool
world_zoom_out (void)
{
	if (state.zoom == 0)
		return false;

	state.size = ZOOM_SIZE(--state.zoom);
	return true;
}

void
world_destroy (void)
{
}

bool
world_init (const unsigned int zoom)
{
	state.zoom = zoom;
	state.size = ZOOM_SIZE(zoom);

	return true;
}
