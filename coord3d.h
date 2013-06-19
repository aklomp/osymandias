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
	double lat = (y / world_size - 0.5) * 2.0 * M_PI;
	return atan(sinh(lat));
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

	*x = cos(lat) * sin(lon - viewlon) * world_radius;
	*z = cos(lat) * cos(lon - viewlon) * world_radius;
	*y = sin(-lat) * world_radius;

	// Rotate the points over lat radians via x axis:
	double yorig = *y;
	double zorig = *z;

	*y = yorig * cos_viewlat - zorig * sin_viewlat;
	*z = yorig * sin_viewlat + zorig * cos_viewlat;

	// Push the world "back" from the camera
	// so that the cursor (centerpoint) is at (0,0,0):
	*z -= world_radius;
}
