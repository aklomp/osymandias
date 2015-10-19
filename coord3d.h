static inline double
world_x_to_lon (double x, double world_size)
{
	// xleft is xmin is -pi, xright is xmax is pi; scale accordingly:
	return (x / world_size - 0.5) * 2.0 * M_PI;
}

static inline double
world_y_to_lat (double y, double world_size)
{
	// ybottom is ymin is -pi, ytop is ymax is pi:
	return atan(sinh((y / world_size - 0.5) * 2.0 * M_PI));
}

static inline double
tile_y_to_lat (double y, double world_size)
{
	return world_y_to_lat(world_size - y, world_size);
}

static inline void
latlon_to_xyz (const double lat, const double lon, const double world_size, const double viewlon, const double sin_viewlat, const double cos_viewlat, double *x, double *y, double *z)
{
	const double world_radius = world_size / M_PI;

	*x = cos(lat) * sin(lon - viewlon);
	*z = cos(lat) * cos(lon - viewlon);
	*y = sin(lat);

	// Rotate the points over lat radians via x axis:
	double yorig = *y;
	double zorig = *z;

	*y = yorig * cos_viewlat - zorig * sin_viewlat;
	*z = yorig * sin_viewlat + zorig * cos_viewlat;

	// Scale the world to its full size:
	*x *= world_radius;
	*y *= world_radius;
	*z *= world_radius;

	// Translate the world "back" from the camera,
	// so that the cursor (centerpoint) is at (0,0,0):
	*z -= world_radius;
}

static inline void
tilepoint_to_xyz_precalc (const double world_size, const double cx, const double cy, double *cx_lon, double *sin_cy_lat, double *cos_cy_lat)
{
	// Precalculate some unchanging values needed by tilepoint_to_xyz below:

	// Get the view longitude/latitude (center point):
	*cx_lon = world_x_to_lon(cx, world_size);
	double cy_lat = tile_y_to_lat(cy, world_size);

	sincos(cy_lat, sin_cy_lat, cos_cy_lat);
}

static inline void
tilepoint_to_xyz (const float x, const float y, const double world_size, const double cx_lon, const double sin_cy_lat, const double cos_cy_lat, float *p)
{
	// Find the lat/lon to which the tilepoint corresponds:
	double lon = world_x_to_lon(x, world_size);
	double lat = tile_y_to_lat(y, world_size);
	double px, py, pz;

	latlon_to_xyz(lat, lon, world_size, cx_lon, sin_cy_lat, cos_cy_lat, &px, &py, &pz);

	p[0] = px;
	p[1] = py;
	p[2] = -pz;
	p[3] = -1.0f;
}
