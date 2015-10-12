#version 110

uniform mat4 mat_frustum;
varying vec4 fpos;

bool inside_frustum (void)
{
	/* Project position through the frustum: */
	vec4 v = mat_frustum * fpos;

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
