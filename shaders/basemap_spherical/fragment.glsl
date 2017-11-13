#version 130

uniform sampler2D tex;

in vec4 p1;
in vec4 p2;

out vec4 fragcolor;

#define M_PI 3.14159265358979323846

void main (void)
{
	// We are working in model space, where the spherical world is a unit
	// sphere centered on the origin. We're casting a ray through p1 and p2
	// and trying to find the intersection with this sphere. Theory here:
	//
	//   https://en.wikipedia.org/wiki/Line%E2%80%93sphere_intersection
	//
	// As the sphere is centered on the origin, the starting point of our
	// ray minus the center of the sphere is simply point p1:
	vec3 start = p1.xyz;

	// The ray's direction is the normalized difference between p1 and p2:
	vec3 delta = normalize(p1 - p2).xyz;

	// Distance of start vector to origin:
	float startdist = length(start);

	// Dot product of direction with start position:
	float raydot = dot(start, delta);

	// Calculate the value under the square root:
	float det = (raydot * raydot) - (startdist * startdist) + 1.0;

	// If this value is negative, no intersection exists:
	if (det < 0.0)
		discard;

	// Get the time value to intersection point:
	float t = sqrt(det) - raydot;

	// If the intersection is behind us, discard:
	if (t >= 0.0)
		discard;

	// Get the intersection point:
	vec3 hit = start + t * delta;

	// Derive lat:
	float lat = atanh(hit.y);

	// Derive tile-y:
	float tile_y = (0.5 - lat / (2.0 * M_PI));

	// Fill holes at poles:
	if (tile_y < 0.0)
		fragcolor = vec4(0.710, 0.816, 0.816, 1.0);

	else if (tile_y > 1.0)
		fragcolor = vec4(0.945, 0.933, 0.910, 1.0);

	else {
		// Derive lon:
		float lon = atan(hit.x, hit.z);

		// Derive tile-x:
		float tile_x = (0.5 + lon / (2.0 * M_PI));

		// Sample the texture:
		fragcolor = texture2D(tex, vec2(tile_x, tile_y));
	}
}
