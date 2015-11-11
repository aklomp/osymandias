#include <stdbool.h>

#include "worlds.h"
#include "worlds/local.h"
#include "worlds/planar.h"
#include "worlds/spherical.h"

// At zoom level 0, the world is 1 tile wide.
// At zoom level 1, it's two tiles.
// At zoom level 2, it's four tiles, etc.
#define ZOOM_SIZE(z)	(1U << (z))
#define ZOOM_MAX	18

// Array of world pointers:
static const struct world *worlds[2];

// Remember current world:
static enum worlds current;

// Current state of world:
static struct {
	unsigned int zoom;
	unsigned int size;
} state;

void
world_set (enum worlds world)
{
	current = world;
}

enum worlds
world_get (void)
{
	return current;
}

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

	worlds[current]->zoom(state.zoom, state.size);
	return true;
}

bool
world_zoom_out (void)
{
	if (state.zoom == 0)
		return false;

	state.size = ZOOM_SIZE(--state.zoom);

	worlds[current]->zoom(state.zoom, state.size);
	return true;
}

void
worlds_destroy (void)
{
}

bool
worlds_init (const unsigned int zoom)
{
	worlds[WORLD_PLANAR] = world_planar();
	worlds[WORLD_SPHERICAL] = world_spherical();

	state.zoom = zoom;
	state.size = ZOOM_SIZE(zoom);

	return true;
}
