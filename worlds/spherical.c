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
	float radius;
} state;

// Various transformation matrices:
static struct {
	struct {
		float lat[16];
		float lon[16];
	} rotate;
	float translate[16];
	float scale[16];
	float model[16];
} matrix;

static void
project (float *vertex, float *normal, const float lat, const float lon)
{
	struct vector *v = (struct vector *)vertex;
	struct vector *n = (struct vector *)normal;

	// Basic lat/lon projection on a unit sphere:
	v->x = cosf(lat) * sinf(lon);
	v->z = cosf(lat) * cosf(lon);
	v->y = sinf(lat);
	v->w = 1.0f;

	// Normal is identical to position, but with zero w:
	n->x = v->x;
	n->y = v->y;
	n->z = v->z;
	n->w = 0.0f;

	// Apply model matrix:
	mat_vec_multiply(vertex, matrix.model, vertex);
	mat_vec_multiply(normal, matrix.model, normal);
}

static void
update_matrix_model (void)
{
	mat_multiply(matrix.model, matrix.rotate.lat, matrix.rotate.lon);
	mat_multiply(matrix.model, matrix.scale, matrix.model);
	mat_multiply(matrix.model, matrix.translate, matrix.model);
}

static void
moveto (const float lat, const float lon)
{
	state.lat = lat;
	state.lon = lon;

	// Rotate longitude into view over y axis:
	mat_rotate(matrix.rotate.lon, 0.0f, 1.0f, 0.0f, lon);

	// Rotate latitude into view over x axis:
	mat_rotate(matrix.rotate.lat, -1.0f, 0.0f, 0.0f, lat);

	// Update model matrix:
	update_matrix_model();
}

static void
zoom (const unsigned int zoom, const unsigned int size)
{
	state.zoom = zoom;
	state.size = size;
	state.radius = (float)size / M_PI;

	// Scale matrix: scale the model (a unit sphere) to the world radius:
	mat_scale(matrix.scale, state.radius, state.radius, state.radius);

	// Translate matrix: push the model back by the world radius:
	mat_translate(matrix.translate, 0.0f, 0.0f, -state.radius);

	// Update model matrix:
	update_matrix_model();
}

static const float *
matrix_model (void)
{
	return matrix.model;
}

const struct world *
world_spherical (void)
{
	static const struct world world = {
		.matrix  = matrix_model,
		.moveto  = moveto,
		.project = project,
		.zoom    = zoom,
	};

	return &world;
}
