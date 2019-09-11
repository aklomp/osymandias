#version 130

uniform mat4 mat_mvp_origin;
uniform vec3 cam;

smooth in vec4 fpos;

out vec4 fragcolor;

const float pi = 3.1415926535897932384626433832795;

float y_to_lat (float y)
{
	return atan(sinh((y - 0.5) * 2.0 * pi));
}

float x_to_lon (float x)
{
	return (x - 0.5) * 2.0 * pi;
}

bool inside_frustum (void)
{
	float lat = y_to_lat(fpos.y);
	float lon = x_to_lon(fpos.x);

	// Point on unit sphere:
	vec4 pos;
	pos.x = cos(lat) * sin(lon);
	pos.z = cos(lat) * cos(lon);
	pos.y = sin(lat);
	pos.w = 1.0;

	// Compare direction of point to camera with the local normal (which,
	// for a unit sphere, are the point coordinates). If the angle is over
	// 90 degrees, the point is not visible to the camera:
	if (dot(cam - pos.xyz, pos.xyz) < 0)
		return false;

	// Project position to eye space:
	pos = mat_mvp_origin * (pos - vec4(cam, 1.0));

	// Projected point is visible if within (-w, w):
	if (any(lessThan(pos.xy, vec2(-pos.w))))
		return false;

	return all(lessThanEqual(pos.xy, vec2(pos.w)));
}

void main (void)
{
	if (!inside_frustum())
		discard;

	fragcolor = vec4(1.0, 0.3, 0.3, 0.5);
}
