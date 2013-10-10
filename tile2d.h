// The zoom decay determines the rate by which the zoom level drops off with distance:
#define ZOOM_DECAY	50.0

static inline vec4f
tile_to_world (const vec4f v, const float center_x, const float center_y)
{
	// Subtract center, then flip y coordinate:
	return (v - (vec4f){ center_x, center_y, 0, 0 }) * (vec4f){ 1, -1, 1, 0 };
}

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
	vec4f x = { x1, x2 - 1, x2 - 1, x1 };
	vec4f y = { y1, y1, y2 - 1, y2 - 1 };

	// Translate tile coordinates to world coordinates:
	x += vec4f_float(0.5 - center_x);
	y += vec4f_float(0.5 - center_y);
	y = -y;

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
tile2d_get_corner_zooms (int x, int y, int sz, float center_x, float center_y, int world_zoom)
{
	return tile2d_get_corner_zooms_abs(x, y, x + sz, y + sz, center_x, center_y, world_zoom);
}

static inline int
tile2d_get_zoom (int x, int y, float center_x, float center_y, int world_zoom)
{
	// Use centerpoint of tile as reference:
	vec4f point = tile_to_world((vec4f){ x + 0.5, y + 0.5, 0, 0 }, center_x, center_y);

	// Calculate squared distance from camera position, scale to zoomlevel:
	int dist = camera_distance_squared_point(point) / ZOOM_DECAY;

	// Find absolute zoomlevel by subtracting it from the current world zoom:
	int zoom = world_zoom - dist;

	// Clip to 0 if negative:
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
