#version 130

uniform mat4 mat_frustum;
uniform mat4 mat_model;
uniform int world_size;
uniform bool spherical;
uniform vec4 camera;
in  vec4 fpos;
out vec4 fragcolor;

#define M_PI 3.1415926535897932384626433832795

float y_to_lat (float y)
{
	return atan(sinh((y / world_size - 0.5) * 2.0 * M_PI));
}

float x_to_lon (float x)
{
	return (x / world_size - 0.5) * 2.0 * M_PI;
}

bool inside_frustum (vec4 pos)
{
	/* Project position through the frustum: */
	vec4 v = mat_frustum * pos;

	/* Projected point is visible if within (-v.w, v.w): */
	if (v.x <= -v.w || v.x >= v.w)
		return false;

	if (v.y <= -v.w || v.y >= v.w)
		return false;

	if (v.z <= -v.w || v.z >= v.w)
		return false;

	return true;
}

bool inside_frustum_spherical (void)
{
	float lat = y_to_lat(fpos.y);
	float lon = x_to_lon(fpos.x);

	/* Point on unit sphere: */
	vec4 pos;
	pos.x = cos(lat) * sin(lon);
	pos.z = cos(lat) * cos(lon);
	pos.y = sin(lat);
	pos.w = 1.0;

	/* Normal is equal to position, but with zero w: */
	vec4 normal = vec4(pos.xyz, 0.0);

	/* Rotate/translate/scale: */
	pos    = mat_model * pos;
	normal = mat_model * normal;

	if (dot(camera.xyz - pos.xyz, normal.xyz) < 0)
		return false;

	return inside_frustum(pos);
}

bool inside_frustum_planar (void)
{
	return inside_frustum(mat_model * fpos);
}

void main (void)
{
	bool inside = (spherical)
		? inside_frustum_spherical()
		: inside_frustum_planar();

	if (!inside)
		discard;

	fragcolor = vec4(1.0, 0.3, 0.3, 0.5);
}
