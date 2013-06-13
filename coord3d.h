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
latlon_to_xyz (double lat, double lon, double world_size, double viewlon, double sin_viewlat, double cos_viewlat, double *x, double *y, double *z)
{
	lon -= viewlon;

	*x = cos(lat) * sin(lon) * world_size;
	*z = cos(lat) * cos(lon) * world_size;
	*y = sin(-lat) * world_size;

	// Rotate the points over lat radians via x axis:
	double yorig = *y;
	double zorig = *z;

	*y = yorig * cos_viewlat - zorig * sin_viewlat;
	*z = yorig * sin_viewlat + zorig * cos_viewlat;

	// Push the world "back" from the camera
	// so that the cursor (centerpoint) is at (0,0,0):
	*z -= world_size;
}
