#include <math.h>

#include "globe.h"
#include "matrix.h"

static struct globe globe;

const struct globe *
globe_get (void)
{
	return &globe;
}

void
globe_moveto (const float lat, const float lon)
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
	mat_rotate(globe.matrix.rotate.lon, 0.0f, 1.0f, 0.0f,  globe.lon);
	mat_rotate(globe.invert.rotate.lon, 0.0f, 1.0f, 0.0f, -globe.lon);

	// Latitudinal rotation matrix:
	mat_rotate(globe.matrix.rotate.lat, -1.0f, 0.0f, 0.0f,  globe.lat);
	mat_rotate(globe.invert.rotate.lat, -1.0f, 0.0f, 0.0f, -globe.lat);

	// Combined model matrix and its inverse:
	mat_multiply(globe.matrix.model, globe.matrix.rotate.lat, globe.matrix.rotate.lon);
	mat_multiply(globe.invert.model, globe.invert.rotate.lon, globe.invert.rotate.lat);
}

void
globe_init (void)
{
	globe_moveto(0.0f, 0.0f);
}
