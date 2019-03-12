#version 130

uniform mat4 mat_viewproj;
uniform mat4 mat_model;
uniform int  tile_zoom;

in  vec2 vertex;
in  vec2 texture;
out vec2 ftex;

const float pi      = 3.141592653589793238462643383279502884;
const float half_pi = 1.570796326794896619231321691639751442;

// For lat/lon conversions, see:
// https://wiki.openstreetmap.org/wiki/Slippy_map_tilenames
//
// Gudermannian function, inverse mercator:
//   gd(y) = atan(sinh(y))
//
// Identities which are used for sphere projection:
//   sin(gd(y)) = tanh(y) = sinh(y) / cosh(y)
//   cos(gd(y)) = sech(y) = 1 / cosh(y)

vec4 sphere_coord (vec2 tile)
{
	// x to longitude is straightforward:
	float lon = tile.x * half_pi * exp2(2 - tile_zoom) - pi;

	// Precalculate the y in gd(y):
	float gy = pi - tile.y * half_pi * exp2(2 - tile_zoom);

	// Translate lat/lon to coordinates on a unit sphere. Avoid an early
	// division of x, y and z by cosh(gy) by setting w to cosh(gy) and
	// making OpenGL do the work in the perspective division:
	return vec4(
		sin(lon),
		sinh(gy),
		cos(lon),
		cosh(gy)
	);
}

void main (void)
{
	ftex = texture;

	// Apply projection matrix to get view coordinates:
	gl_Position = mat_viewproj * mat_model * sphere_coord(vertex);

	// Zoom level determines screen Z depth,
	// range is -1 (nearest) to 1 (farthest), though the basemap is zero:
	gl_Position.z  = float(tile_zoom) / -20.0 - 0.01;
	gl_Position.z *= gl_Position.w;
}
