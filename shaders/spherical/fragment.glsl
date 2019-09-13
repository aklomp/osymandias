#version 130

uniform sampler2D tex;
uniform int       tile_y;
uniform int       tile_zoom;

flat in vec3  tile_origin;
flat in vec3  tile_xaxis;
flat in vec3  tile_yaxis;
flat in vec3  tile_normal;
flat in float tile_xlength0;
flat in float tile_xlength1;
flat in float tile_ylength;

flat   in vec3   cam;
smooth in mat4x3 rays;

out vec4 fragcolor;

mat4x3 mat4x3vec (in vec3 v)
{
	return mat4x3(v, v, v, v);
}

mat4x3 sphere_intersect (in mat4x3 nrays)
{
	// Find the intersection of a set of normalized rays starting at the
	// camera, with a unit sphere centered on the origin. Theory:
	//   https://en.wikipedia.org/wiki/Line%E2%80%93sphere_intersection

	// Squared distance of the camera to the world origin:
	float camdist = dot(cam, cam);

	// Dot products of the camera (ray start) position with the rays:
	vec4 raydot = cam * nrays;

	// Get the time values to intersection point. The value under the
	// square root is always positive by definition. Rays are only cast to
	// tiles, which are known to be fully contained within the sphere:
	vec4 t = sqrt(raydot * raydot - camdist + 1.0) + raydot;

	// Get the intersection point in world coordinates:
	return mat4x3vec(cam) - matrixCompMult(transpose(mat3x4(t, t, t)), nrays);
}

// Transform coordinates on a unit sphere to texture coordinates, using the
// vector definitions of the tile supplied by the vertex shader. The goal is to
// use geometric constructions to avoid trigonometry as much as possible.
uvec4 sphere_to_texture (in mat4x3 s, out vec4 tx, out vec4 ty)
{
	uvec4 valid;

	// Get the position of the sphere points relative to the tile origin:
	mat4x3 tile = s - mat4x3vec(tile_origin);

	// Project the sphere point onto the tile plane by subtracting the tiny
	// distance between sphere surface and plane, using the hit point
	// itself as the direction vector (it is by definition normalized):
	//   https://en.wikipedia.org/wiki/Line%E2%80%93plane_intersection
	vec4 a = (tile_normal * tile) / (tile_normal * s);
	tile -= matrixCompMult(s, transpose(mat3x4(a, a, a)));

	// Convert the y component to a fraction:
	ty = (tile_yaxis * tile) / tile_ylength;

	// Because the tile is rendered as an isosceles trapezoid, the x
	// component is scaled according to the x width at the current height:
	vec4 xwidth = mix(vec4(tile_xlength0), vec4(tile_xlength1), ty);

	// Convert the x component to a fraction, compensate for the origin
	// being in the center of the tile:
	tx = 0.5 + (tile_xaxis * tile) / xwidth;

	// Range checks, using multiplication as a poor man's `and':
	valid  = uvec4(greaterThanEqual(tx, vec4(0.0)));
	valid *= uvec4(lessThan        (tx, vec4(1.0)));

	// At high zoom levels, the distortion in the y direction due to the
	// Mercator projection becomes virtually unnoticeable within a single
	// tile. The y gradient over the tile becomes essentially linear. Take
	// a shortcut and treat those tiles as actually linear. This avoids a
	// costly logarithm, and sidesteps floating-point accuracy problems:
	if (tile_zoom > 12) {
		valid *= uvec4(greaterThanEqual(ty, vec4(0.0)));
		valid *= uvec4(lessThan        (ty, vec4(1.0)));
		return valid;
	}

	// For lower zoom levels, reach for trigonometry to transform the y
	// coordinate accurately. The naive formula is:
	//
	//   2^zoom * (0.5 - atanh(y) / (2 * pi))
	//
	// which can be reduced by combining terms and baking factors of two
	// into the constant. Observing that:
	//
	//   -2 * atanh(y) = log((1 - y) / (1 + y))
	//                 = log2((1 - y) / (1 + y)) / log2(e)
	//
	// and that the integer offset can be ignored because we're only
	// interested in the fractional part of the equation, the naive formula
	// can ultimately be rewritten as something less daunting.
	const float yconst = 0.882542400610606373585825728472;	// 4 / pi / log2(e)
	vec4 sy = transpose(s)[1];

	ty = yconst * log2((1.0 - sy) / (1.0 + sy)) * exp2(tile_zoom - 4);

	// Re-add the integer offset to get the full y coordinate:
	vec4 tyfull = ty + exp2(tile_zoom - 1);

	// Test the full y coordinate against tile extents:
	valid *= uvec4(greaterThanEqual(tyfull, vec4(tile_y + 0)));
	valid *= uvec4(lessThan        (tyfull, vec4(tile_y + 1)));

	return valid;
}

void main (void)
{
	float nsamples;
	vec4 tx, ty;

	// Cast rays from the camera and intersect with the unit sphere:
	mat4x3 s = sphere_intersect(mat4x3(
		normalize(rays[0]),
		normalize(rays[1]),
		normalize(rays[2]),
		normalize(rays[3])));

	// Get texture coordinates:
	uvec4 valid = sphere_to_texture(s, tx, ty);

	// Count number of valid samples:
	if ((nsamples = dot(valid, valid)) == 0.0)
		discard;

	// Texture lookup:
	mat4x4 tsamples = mat4x4(
		texture(tex, vec2(tx[0], ty[0])),
		texture(tex, vec2(tx[1], ty[1])),
		texture(tex, vec2(tx[2], ty[2])),
		texture(tex, vec2(tx[3], ty[3])));

	// Square the values:
	tsamples = matrixCompMult(tsamples, tsamples);

	// Multiply by validity vector and sum the result:
	vec4 sum = tsamples * valid;

	// Use square root of the average as the sample value:
	fragcolor = sqrt(sum / nsamples);
}
