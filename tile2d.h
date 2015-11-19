// The zoom decay determines the rate by which the zoom level drops off with distance:
#define ZOOM_DECAY	50.0

static inline vec4i
tile2d_get_corner_zooms_abs (int x1, int y1, int x2, int y2, const struct center *center, int world_zoom)
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
	vec4f x = { x1, x2 - 1, x2 - 1, x1 };
	vec4f y = { y1, y1, y2 - 1, y2 - 1 };

	// Translate tile coordinates to world coordinates:
	x += vec4f_float(0.5 - center->tile.x);
	y += vec4f_float(0.5 - center->tile.y);

	// Use distance-squared, save a square root:
	vec4f d = camera_distance_squared_quad(x, y, (vec4f){ 0, 0, 0, 0 });

	// Scale the distances down to zoom levels and convert to int:
	vec4i vz = vec4f_to_vec4i(d / vec4f_float(ZOOM_DECAY));

	// Turn distance gradients into absolute zoom levels:
	vz = vec4i_int(world_zoom) - vz;

	// Clip negative absolute zooms to zero:
	return vz & (vz > vec4i_zero());
}

static inline vec4i
tile2d_get_corner_zooms (int x, int y, int sz, const struct center *center, int world_zoom)
{
	return tile2d_get_corner_zooms_abs(x, y, x + sz, y + sz, center, world_zoom);
}

static inline int
tile2d_get_zoom (int x, int y, int world_zoom)
{
	// Use centerpoint of tile as reference:
	vec4f point = (vec4f){ x + 0.5, world_get_size() - (y + 0.5), 0, 0 };

	// Calculate squared distance from camera position, scale to zoomlevel:
	int dist = camera_distance_squared((struct vector *)&point) / ZOOM_DECAY;

	// Find absolute zoomlevel by subtracting it from the current world zoom:
	int zoom = world_zoom - dist;

	// Clip to 0 if negative:
	return (zoom < 0) ? 0 : zoom;
}

static inline int
tile2d_get_max_zoom (int x, int y, int sz, const struct center *center, int world_zoom)
{
	// For the given quad, find the tile nearest to center:
	int rx = (center->tile.x > x + sz) ? x + sz : (center->tile.x < x) ? x : center->tile.x + 0.5;
	int ry = (center->tile.y > y + sz) ? y + sz : (center->tile.y < y) ? y : center->tile.y + 0.5;

	return tile2d_get_zoom(rx, ry, world_zoom);
}

static inline vec4i
zooms_quad (int world_zoom, const vec4f x, const vec4f y, const vec4f z)
{
	// Get distance squared from camera location:
	vec4f d = camera_distance_squared_quad(x, y, z);

	// Scale down to zoom gradients, convert to int:
	vec4i vz = vec4f_to_vec4i(d / vec4f_float(ZOOM_DECAY));

	// Turn distance gradient into absolute zoom levels:
	vz = vec4i_int(world_zoom) - vz;

	// Clip negative zooms to zero:
	return vz & (vz > vec4i_zero());
}

static inline int
zoom_edges_highest (int world_zoom, const vec4f x, const vec4f y, const vec4f z)
{
	// Get squared distances from camera to each edge line:
	vec4f d = camera_distance_squared_quadedge(x, y, z);

	// Scale down to zoom gradients, convert to int:
	vec4i vz = vec4f_to_vec4i(d / vec4f_float(ZOOM_DECAY));

	// Turn into absolute zoom levels:
	vz = vec4i_int(world_zoom) - vz;

	// Clip negative zooms to zero:
	vz &= (vz > vec4i_zero());

	return vec4i_hmax(vz);
}

static inline int
zoom_point (int world_zoom, const struct vector *v)
{
	float f = camera_distance_squared(v);
	int r = world_zoom - (f / ZOOM_DECAY);

	return (r < 0) ? 0 : r;
}
