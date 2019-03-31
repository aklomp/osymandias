#version 130

uniform sampler2D tex;
uniform int       tile_zoom;

flat in vec3  tile_origin;
flat in vec3  tile_xaxis;
flat in vec3  tile_yaxis;
flat in vec3  tile_normal;
flat in float tile_xlength0;
flat in float tile_xlength1;
flat in float tile_ylength;

flat   in vec3 cam;
smooth in vec3 p;

out vec4 fragcolor;

// Transform coordinates on a unit sphere to texture coordinates, using the
// vector definitions of the tile supplied by the vertex shader. The goal is to
// use geometric constructions to avoid trigonometry as much as possible.
bool sphere_to_texture (in vec3 s, out vec2 t)
{
	// Get the position of the sphere point relative to the tile origin:
	vec3 tile = s - tile_origin;

	// Project the sphere point onto the tile plane by subtracting the tiny
	// distance between sphere surface and plane, using the hit point
	// itself as the direction vector (it is by definition normalized):
	//   https://en.wikipedia.org/wiki/Line%E2%80%93plane_intersection
	tile -= s * (dot(tile, tile_normal) / dot(s, tile_normal));

	// Convert the y component to a fraction:
	t.y = dot(tile, tile_yaxis) / tile_ylength;

	// Range check:
	if (t.y < 0.0 || t.y >= 1.0)
		return false;

	// Because the tile is rendered as an isosceles trapezoid, the x
	// component is scaled according to the x width at the current height:
	float xwidth = mix(tile_xlength0, tile_xlength1, t.y);

	// Convert the x component to a fraction, compensate for the origin
	// being in the center of the tile:
	t.x = 0.5 + dot(tile, tile_xaxis) / xwidth;

	// Range check:
	return t.x >= 0.0 && t.x < 1.0;
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
	vec3 s;
	vec2 texcoords;

	// Cast a ray from the camera to p and intersect with the unit sphere:
	if (sphere_intersect(cam, normalize(cam - p), s) == false)
		discard;

	// Get texture coordinates:
	if (sphere_to_texture(s, texcoords) == false)
		discard;

	// Texture lookup:
	fragcolor = texture(tex, texcoords);
}
