#version 130

noperspective in vec3 p1;
noperspective in vec3 p2;

out uvec4 fragcolor;

const float pi = 3.141592653589793238462643383279502884;

const int zoom_min = 0;
const int zoom_max = 19;

// Transform coordinates on a unit sphere to tile coordinates at a given zoom.
bool sphere_to_tile (in vec3 s, in int zoom, out vec2 tile)
{
	// Start with the y coordinate, since it can be out of range in the
	// polar zones. The naive formula is:
	//
	//   2^zoom * (0.5 - atanh(y) / (2 * pi))
	//
	// which can be reduced by combining terms. Observing that:
	//
	//   2 * atanh(y) = log((1 + y) / (1 - y))
	//
	// the naive formula can be rewritten as:
	//
	//   2^(zoom - 1) * (1 + log((1 - y) / (1 + y)) / (2 * pi))
	//
	// Start with the invariant term:
	float y = log((1.0 - s.y) / (1.0 + s.y)) / 2.0 / pi;

	// Reject polar zones:
	if (abs(y) >= 1.0)
		return false;

	// Complete the multiplication:
	tile.y = exp2(zoom - 1) * (1.0 + y);

	// The naive formula for the x coordinate is:
	//
	//  2^zoom * (0.5 + atan(x / z) / (2 * pi))
	//
	// This expression can also be simplified by combining terms:
	tile.x = exp2(zoom - 1) * (1.0 + atan(s.x, s.z) / pi);

	return true;
}

int zoomlevel (float dist, float lat)
{
	// At high latitudes, tiles are smaller and closer together. Decrease
	// the zoom strength based on latitude to keep the number of tiles
	// down. The divisor is an arbitrary number that determines the
	// strength of the attenuation as latitude increases:
	float latdilute = 1.0 - abs(lat) / 12.0;

	// Get zoomlevel based on the distance to the camera. The zoom level is
	// inversely proportional to the square of the distance. The smaller
	// the distance, the greater the zoom level. Take the exponent of the
	// distance and flip the sign. Subtract from a constant to tune for the
	// camera view angle and distance at zoom level zero:
	int zoom = int((5 - log2(dist)) * latdilute);

	return clamp(zoom, zoom_min, zoom_max);
}

bool sphere_intersect (in vec3 start, in vec3 dir, out vec3 hit)
{
	// Find the intersection of a ray with the given start point and
	// direction with a unit sphere centered on the origin. Theory here:
	//
	//   https://en.wikipedia.org/wiki/Line%E2%80%93sphere_intersection
	//
	// Distance of ray origin to world origin:
	float startdist = length(start);

	// Dot product of start position with direction:
	float raydot = dot(start, dir);

	// Calculate the value under the square root:
	float det = (raydot * raydot) - (startdist * startdist) + 1.0;

	// If this value is negative, no intersection exists:
	if (det < 0.0)
		return false;

	// Get the time value to intersection point:
	float t = sqrt(det) - raydot;

	// If the intersection is behind us, discard:
	if (t >= 0.0)
		return false;

	// Get the intersection point:
	hit = start + t * dir;
	return true;
}

void main (void)
{
	vec3 hit;
	vec2 tile;

	// Cast a ray from p1 and p2 and intersect it with the unit sphere:
	if (sphere_intersect(p1, normalize(p1 - p2), hit) == false) {
		fragcolor = uvec4(0);
		return;
	}

	// Get zoomlevel based on distance to camera:
	int zoom = zoomlevel(distance(hit, p1), atanh(hit.y));

	if (sphere_to_tile(hit, zoom, tile) == false) {
		fragcolor = uvec4(0);
		return;
	}

	fragcolor = uvec4(int(tile.x), int(tile.y), zoom, 1);
}
