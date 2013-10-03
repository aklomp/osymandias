static inline vec4i
tile2d_get_corner_zooms_abs (int x1, int y1, int x2, int y2, float center_x, float center_y, int world_zoom)
{
	// The corner tiles are the 1x1 unit tiles at the edges of this larger
	// square tile; the distance from the center point determines the zoom level
	// at which they should be shown:
	//
	// x1 -> x2
	// +-----+ y1
	// |'   '| |     (center_x, center_y)
	// |,   ,| V              .
	// +-----+ y2
	//
	vec4i zero = vec4i_zero();
	vec4f vx = { x1, x2 - 1, x2 - 1, x1 };
	vec4f vy = { y1, y1, y2 - 1, y2 - 1 };

	// Use distance-squared, save a square root;
	// the 40.0 is a fudge factor for the size of the "zoom horizon":
	vx = vector2d_distance_squared(center_x - 0.5, center_y - 0.5, vx, vy) / vec4f_float(40.0);

	// Convert to int:
	vec4i vz = vec4f_to_vec4i(vx);

	// Turn distance gradients into absolute zoom levels:
	vz = vec4i_int(world_zoom) - vz;

	// Clip negative absolute zooms to zero:
	return vz & (vz > zero);
}

static inline vec4i
tile2d_get_corner_zooms (int x, int y, int sz, float center_x, float center_y, int world_zoom)
{
	return tile2d_get_corner_zooms_abs(x, y, x + sz, y + sz, center_x, center_y, world_zoom);
}

static inline int
tile2d_get_zoom (int x, int y, float center_x, float center_y, int world_zoom)
{
	float dx = (float)x - center_x + 0.5;
	float dy = (float)y - center_y + 0.5;

	int dist = (dx * dx + dy * dy) / 40.0;
	int zoom = world_zoom - dist;

	return (zoom < 0) ? 0 : zoom;
}

static inline int
tile2d_get_max_zoom (int x, int y, int sz, float center_x, float center_y, int world_zoom)
{
	// For the given quad, find the tile nearest to center:
	int rx = (center_x > x + sz) ? x + sz : (center_x < x) ? x : center_x + 0.5;
	int ry = (center_y > y + sz) ? y + sz : (center_y < y) ? y : center_y + 0.5;

	return tile2d_get_zoom(rx, ry, center_x, center_y, world_zoom);
}
