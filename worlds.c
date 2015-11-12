#include <stdbool.h>
#include <math.h>

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
static struct world_state state;

void
world_tile_to_latlon (float *lat, float *lon, const float x, const float y)
{
	*lat = atanf(sinhf((0.5f - y / state.size) * 2.0f * M_PI));
	*lon = (x / state.size - 0.5f) * 2.0f * M_PI;
}

void
world_latlon_to_tile (float *x, float *y, const float lat, const float lon)
{
	*x = state.size * (0.5f + lon / (2.0f * M_PI));
	*y = state.size * (0.5f - atanhf(sinf(lat)) / (2.0f * M_PI));
}

void
world_set (const enum worlds world)
{
	current = world;

	// Update new world's zoom and position:
	worlds[current]->move(&state);
	worlds[current]->zoom(&state);
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

	worlds[current]->zoom(&state);
	return true;
}

bool
world_zoom_out (void)
{
	if (state.zoom == 0)
		return false;

	state.size = ZOOM_SIZE(--state.zoom);

	worlds[current]->zoom(&state);
	return true;
}

void
world_moveto_tile (const float x, const float y)
{
	// Save new lat/lon:
	world_tile_to_latlon(&state.lat, &state.lon, x, y);

	// Move:
	worlds[current]->move(&state);
}

void
world_moveto_latlon (const float lat, const float lon)
{
	// Save new lat/lon:
	state.lat = lat;
	state.lon = lon;

	// Move:
	worlds[current]->move(&state);
}

void
world_project_tile (float *vertex, float *normal, const float x, const float y)
{
	float lat, lon;

	// Convert to lat/lon:
	world_tile_to_latlon(&lat, &lon, x, y);

	// Project:
	worlds[current]->project(&state, vertex, normal, lat, lon);
}

void
world_project_latlon (float *vertex, float *normal, const float lat, const float lon)
{
	// Project:
	worlds[current]->project(&state, vertex, normal, lat, lon);
}

void
worlds_destroy (void)
{
}

bool
worlds_init (const unsigned int zoom, const float lat, const float lon)
{
	worlds[WORLD_PLANAR] = world_planar();
	worlds[WORLD_SPHERICAL] = world_spherical();

	state.lat = lat;
	state.lon = lon;
	state.zoom = zoom;
	state.size = ZOOM_SIZE(zoom);

	return true;
}
