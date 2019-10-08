#include <math.h>

#include "globe.h"
#include "matrix.h"
#include "vec.h"

static struct globe globe;

static void
map_point (const struct cache_node *n, struct globe_point *p)
{
	// Convert tile coordinates at a given zoom level to 3D xyz coordinates
	// on a unit sphere. For lat/lon conversions, see:
	//   https://wiki.openstreetmap.org/wiki/Slippy_map_tilenames
	//
	// Gudermannian function, inverse mercator:
	//   gd(y) = atan(sinh(y))
	//
	// Identities which are used for sphere projection:
	//   sin(gd(y)) = tanh(y) = sinh(y) / cosh(y)
	//   cos(gd(y)) = sech(y) = 1 / cosh(y)

	// x to longitude is straightforward:
	const double lon = M_PI * (ldexp(n->x, 1 - n->zoom) - 1.0);

	// Precalculate the y in gd(y):
	const double gy = M_PI * (1.0 - ldexp(n->y, 1 - n->zoom));

	// Precalculate cosh(gy):
	const double cosh_gy = cosh(gy);

	// Precalculate sin(lon) and cos(lon):
	double sin_lon, cos_lon;
	sincos(lon, &sin_lon, &cos_lon);

	// Calculate 3D coordinates on a unit sphere:
	p->x = sin_lon / cosh_gy;
	p->y = tanh(gy);
	p->z = cos_lon / cosh_gy;
}

// Convert tile coordinates to xyz coordinates on a unit sphere.
void
globe_map_tile (const struct cache_node *n, struct globe_tile *t)
{
	map_point(&(struct cache_node) { .x = n->x + 0, .y = n->y + 1, .zoom = n->zoom }, &t->sw);
	map_point(&(struct cache_node) { .x = n->x + 1, .y = n->y + 1, .zoom = n->zoom }, &t->se);
	map_point(&(struct cache_node) { .x = n->x + 1, .y = n->y + 0, .zoom = n->zoom }, &t->ne);
	map_point(&(struct cache_node) { .x = n->x + 0, .y = n->y + 0, .zoom = n->zoom }, &t->nw);
}

// Perform a line-sphere intersection between a unit sphere on the origin, and
// a ray starting at `start', with direction `dir'. Theory here:
//
//   https://en.wikipedia.org/wiki/Line%E2%80%93sphere_intersection
//
static inline bool
intersect (const union vec start, const union vec dir, float *lat, float *lon)
{
	// Squared distance of ray start to origin:
	const float startdist = vec_dot(start, start);

	// Dot product of ray start with ray direction:
	const float raydot = vec_dot(start, dir);

	// The determinant under the square root:
	const float det = (raydot * raydot) - startdist + 1.0f;

	// If the determinant is negative, no intersection exists:
	if (det < 0.0f)
		return false;

	// Get the time value to the nearest intersection point:
	const float t = sqrtf(det) - raydot;

	// Calculate the intersection point:
	const union vec hit = vec_add(start, vec_mul(vec_1(t), dir));

	// Convert intersection point into lat/lon:
	*lat = asinf(hit.y);
	*lon = atan2f(hit.x, hit.z);

	return true;
}

bool
globe_intersect (const union vec *start, const union vec *dir, float *lat, float *lon)
{
	return intersect(*start, vec_normalize(*dir), lat, lon);
}

const struct globe *
globe_get (void)
{
	return &globe;
}

void globe_updated_reset (void)
{
	globe.updated.model = false;
}

void
globe_moveto (const double lat, const double lon)
{
	globe.lat = lat;
	globe.lon = lon;

	if (lat < -M_PI_2)
		globe.lat = -M_PI_2;

	if (lat > M_PI_2)
		globe.lat = M_PI_2;

	if (lon < -M_PI)
		globe.lon += 2.0 * M_PI;

	if (lon > M_PI)
		globe.lon -= 2.0 * M_PI;

	// Longitudinal rotation matrix:
	mat_rotate(globe.matrix.rotate.lon, 0.0, 1.0, 0.0,  globe.lon);
	mat_rotate(globe.invert.rotate.lon, 0.0, 1.0, 0.0, -globe.lon);

	// Latitudinal rotation matrix:
	mat_rotate(globe.matrix.rotate.lat, -1.0, 0.0, 0.0,  globe.lat);
	mat_rotate(globe.invert.rotate.lat, -1.0, 0.0, 0.0, -globe.lat);

	// Combined model matrix and its inverse:
	mat_multiply(globe.matrix.model, globe.matrix.rotate.lat, globe.matrix.rotate.lon);
	mat_multiply(globe.invert.model, globe.invert.rotate.lon, globe.invert.rotate.lat);

	globe.updated.model = true;
}

void
globe_init (void)
{
	globe_moveto(0.0, 0.0);
}
