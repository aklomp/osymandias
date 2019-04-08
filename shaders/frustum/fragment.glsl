#version 130

uniform mat4 mat_viewproj;
uniform mat4 mat_model;
uniform int world_size;
uniform vec4 camera;

smooth in vec4 fpos;

out vec4 fragcolor;

const float pi = 3.1415926535897932384626433832795;

float y_to_lat (float y)
{
	return atan(sinh((y / world_size - 0.5) * 2.0 * pi));
}

float x_to_lon (float x)
{
	return (x / world_size - 0.5) * 2.0 * pi;
}

bool inside_frustum (void)
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

	/* Project position through the frustum: */
	vec4 v = mat_viewproj * pos;

	/* Projected point is visible if within (-v.w, v.w): */
	if (any(lessThan(v.xyz, vec3(-v.w))))
		return false;

	return all(lessThan(v.xyz, vec3(v.w)));
}

void main (void)
{
	if (!inside_frustum())
		discard;

	fragcolor = vec4(1.0, 0.3, 0.3, 0.5);
}
