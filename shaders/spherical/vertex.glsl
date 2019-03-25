#version 130

uniform mat4 mat_viewproj;
uniform mat4 mat_model;
uniform int  world_zoom;
in  vec2 vertex;
in  vec2 texture;
out vec2 ftex;

const float pi = 3.14159265358979323846;

// For lat/lon conversions, see:
// https://wiki.openstreetmap.org/wiki/Slippy_map_tilenames
//
// Gudermannian function, inverse mercator:
//   gd(y) = atan(sinh(y))
//
// Identities which are used for sphere projection:
//   sin(gd(y)) = tanh(y)
//   cos(gd(y)) = sech(y) = 1 / cosh(y)

void main (void)
{
	ftex = texture;

	// x to longitude is straightforward:
	float lon = (vertex.x / (1 << world_zoom) - 0.5) * 2.0 * pi;

	// Precalculate the y in gd(y):
	float y = (1.0 - 2.0 * (vertex.y / (1 << world_zoom))) * pi;

	// Translate lat/lon to coordinates on a unit sphere:
	vec4 spherical = vec4(
		sin(lon) / cosh(y),
		tanh(y),
		cos(lon) / cosh(y),
		1.0
	);

	// Apply projection matrix to get view coordinates:
	gl_Position = mat_viewproj * mat_model * spherical;
}
