#pragma once

#include <stdbool.h>
#include <stdint.h>

#include <vec/vec.h>

enum worlds {
	WORLD_PLANAR,
	WORLD_SPHERICAL,
};

struct coords {
	float lat;
	float lon;
	struct {
		float x;
		float y;
	} tile;
};

// Change current world:
void world_set (const enum worlds world);

// Get current world:
enum worlds world_get (void);

// Get world properties:
unsigned int world_get_size (void);
unsigned int world_get_zoom (void);
const float *world_get_matrix (void);
const float *world_get_matrix_inverse (void);

// Zoom one step in/out of world:
bool world_zoom_in (void);
bool world_zoom_out (void);

// Move cursor to this point, in tile coordinates:
void world_moveto_tile (const float x, const float y);

// Move cursor to this point, in lat/lon:
void world_moveto_latlon (const float lat, const float lon);

// Project this tile coordinate to world coordinates:
void world_project_tile (union vec *vertex, const float x, const float y);

// Project this lat/lon to world coordinates:
void world_project_latlon (union vec *vertex, const float lat, const float lon);

// Convert tile coordinates to lat/lon:
void world_tile_to_latlon (float *lat, float *lon, const float x, const float y);

// Get center coordinate:
const struct coords *world_get_center (void);

// Update world state on timer tick:
bool world_timer_tick (int64_t usec);

// Initialize worlds:
bool worlds_init (const unsigned int zoom, const float lat, const float lon);

// Destroy worlds:
void worlds_destroy (void);

// Autoscroll functions:
void world_autoscroll_measure_down (int64_t usec);
void world_autoscroll_measure_hold (int64_t usec);
void world_autoscroll_measure_free (int64_t usec);
bool world_autoscroll_stop (void);
bool world_autoscroll_update (double *restrict x, double *restrict y, int64_t usec);
