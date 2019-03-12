#include <math.h>

#include "worlds.h"
#include "worlds/local.h"
#include "worlds/autoscroll.h"
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
	*lat = atanf(sinhf((float) M_PI * (1.0f - 2.0f * (y / state.size))));
	*lon = (x / state.size - 0.5f) * 2.0f * (float) M_PI;
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

const float *
world_get_matrix (void)
{
	return worlds[current]->matrix();
}

const float *
world_get_matrix_inverse (void)
{
	return worlds[current]->matrix_inverse();
}

static inline void
coords_zoom_in (struct coords *coords)
{
	coords->tile.x *= 2.0f;
	coords->tile.y *= 2.0f;
}

static inline void
coords_zoom_out (struct coords *coords)
{
	coords->tile.x /= 2.0f;
	coords->tile.y /= 2.0f;
}

bool
world_zoom_in (void)
{
	if (state.zoom == ZOOM_MAX)
		return false;

	state.size = ZOOM_SIZE(++state.zoom);

	// When zooming in, double tile coordinates:
	coords_zoom_in(&state.center);
	coords_zoom_in(&state.autoscroll.down.coords);
	coords_zoom_in(&state.autoscroll.hold.coords);
	coords_zoom_in(&state.autoscroll.free.coords);

	// Halve autoscroll speed as measured in lat/lon:
	state.autoscroll.speed.lat /= 2.0f;
	state.autoscroll.speed.lon /= 2.0f;

	worlds[current]->zoom(&state);
	return true;
}

bool
world_zoom_out (void)
{
	if (state.zoom == 0)
		return false;

	state.size = ZOOM_SIZE(--state.zoom);

	// When zooming in, halve tile coordinates:
	coords_zoom_out(&state.center);
	coords_zoom_out(&state.autoscroll.down.coords);
	coords_zoom_out(&state.autoscroll.hold.coords);
	coords_zoom_out(&state.autoscroll.free.coords);

	// Double autoscroll speed as measured in lat/lon:
	state.autoscroll.speed.lat *= 2.0f;
	state.autoscroll.speed.lon *= 2.0f;

	worlds[current]->zoom(&state);
	return true;
}

void
world_moveto_tile (const float x, const float y)
{
	// Save new lat/lon:
	state.center.tile.x = x;
	state.center.tile.y = y;

	// Restrict tile coordinates to world:
	worlds[current]->center_restrict_tile(&state);

	// Update lat/lon representation:
	world_tile_to_latlon(&state.center.lat, &state.center.lon, state.center.tile.x, state.center.tile.y);

	// Move:
	worlds[current]->move(&state);
}

void
world_moveto_latlon (const float lat, const float lon)
{
	// Save new lat/lon:
	state.center.lat = lat;
	state.center.lon = lon;

	// Restrict lat/lon coordinates to world:
	worlds[current]->center_restrict_latlon(&state);

	// Update tile representation:
	world_latlon_to_tile(&state.center.tile.x, &state.center.tile.y, state.center.lat, state.center.lon);

	// Move:
	worlds[current]->move(&state);
}

void
world_project_tile (union vec *vertex, const float x, const float y)
{
	float lat, lon;

	// Convert to lat/lon:
	world_tile_to_latlon(&lat, &lon, x, y);

	// Project:
	worlds[current]->project(&state, vertex, lat, lon);
}

void
world_project_latlon (union vec *vertex, const float lat, const float lon)
{
	// Project:
	worlds[current]->project(&state, vertex, lat, lon);
}

const struct coords *
world_get_center (void)
{
	return &state.center;
}

bool
world_timer_tick (int64_t usec)
{
	return worlds[current]->timer_tick(&state, usec);
}

// Dispatch autoscroll functions to local handler:
void
world_autoscroll_measure_down (int64_t usec)
{
	autoscroll_measure_down(&state, usec);
}

void
world_autoscroll_measure_hold (int64_t usec)
{
	autoscroll_measure_hold(&state, usec);
}

void
world_autoscroll_measure_free (int64_t usec)
{
	autoscroll_measure_free(&state, usec);
}

bool
world_autoscroll_stop (void)
{
	return autoscroll_stop(&state);
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

	state.zoom = zoom;
	state.size = ZOOM_SIZE(zoom);
	state.center.lat = lat;
	state.center.lon = lon;
	world_latlon_to_tile (&state.center.tile.x, &state.center.tile.y, lat, lon);

	return true;
}
