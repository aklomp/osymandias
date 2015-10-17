#version 130

uniform mat4 mat_frustum;
uniform int world_size;
uniform bool spherical;
uniform float cx;
uniform float cy;
varying vec4 fpos;

#define M_PI 3.1415926535897932384626433832795

float y_to_lat (float y)
{
	float lat = (y / world_size - 0.5) * 2.0 * M_PI;
	return atan(sinh(lat));
}

float x_to_lon (float x)
{
	return (x / world_size - 0.5) * 2.0 * M_PI;
}

vec4 latlon_to_pos (float lat, float lon, float viewlat, float viewlon)
{
	float world_radius = world_size / M_PI;
	vec4 pos;

	pos.x = cos(lat) * sin(lon - viewlon) * world_radius;
	pos.z = cos(lat) * cos(lon - viewlon) * world_radius;
	pos.y = sin(lat) * world_radius;
	pos.w = 1.0;

	/* Rotate the points over lat radians via x axis: */
	vec4 orig = pos;

	pos.y = orig.y * cos(viewlat) - orig.z * sin(viewlat);
	pos.z = orig.y * sin(viewlat) + orig.z * cos(viewlat);

	/* Push the world "back" from the camera
	 * so that the cursor (centerpoint) is at (0,0,0): */
	pos.z -= world_radius;

	return pos;
}

bool inside_frustum (void)
{
	vec4 pos = fpos;

	/* If point is not planar, must map to sphere: */
	if (spherical) {

		float lat = y_to_lat(fpos.y);
		float lon = x_to_lon(fpos.x);

		float viewlat = y_to_lat(cy);
		float viewlon = x_to_lon(cx);

		pos = latlon_to_pos(lat, lon, viewlat, viewlon);
	}

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

void main (void)
{
	gl_FragColor = (inside_frustum())
		? vec4(1.0, 0.3, 0.3, 0.5)
		: vec4(0.0);
}
