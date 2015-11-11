#include <stdbool.h>
#include <stdint.h>
#include <math.h>

#include "../matrix.h"
#include "../vector.h"
#include "../worlds.h"
#include "local.h"

// World attributes:
static struct {
	unsigned int zoom;
	unsigned int size;
	float lat;
	float lon;
} state;

// Transformation matrices:
static struct {
	float model[16];
} matrix;

static inline void
latlon_to_world (float *x, float *y, const float lat, const float lon)
{
	*x = state.size * (0.5f + lon / (2.0f * M_PI));
	*y = state.size * (0.5f + atanhf(sinf(lat)) / (2.0f * M_PI));
}

static void
project (float *vertex, float *normal, const float lat, const float lon)
{
	struct vector *v = (struct vector *)vertex;
	struct vector *n = (struct vector *)normal;

	// Convert lat/lon to world coordinates:
	latlon_to_world(&v->x, &v->y, lat, lon);

	v->z = 0.0f;
	v->w = 1.0f;

	n->x = 0.0f;
	n->y = 0.0f;
	n->z = 1.0f;
	n->w = 0.0f;

	// Apply model matrix:
	mat_vec_multiply(&v->x, matrix.model, &v->x);
	mat_vec_multiply(&n->x, matrix.model, &n->x);
}

static void
update_matrix_model (void)
{
	float x, y;

	// Get world coordinates of current position:
	latlon_to_world(&x, &y, state.lat, state.lon);

	// Translate world so that this position is at origin:
	mat_translate(matrix.model, -x, -y, 0.0f);
}

static void
moveto (const float lat, const float lon)
{
	state.lat = lat;
	state.lon = lon;

	update_matrix_model();
}

static void
zoom (const unsigned int zoom, const unsigned int size)
{
	state.zoom = zoom;
	state.size = size;

	update_matrix_model();
}

static const float *
matrix_model (void)
{
	return matrix.model;
}

const struct world *
world_planar (void)
{
	static const struct world world = {
		.matrix  = matrix_model,
		.moveto  = moveto,
		.project = project,
		.zoom    = zoom,
	};

	return &world;
}
