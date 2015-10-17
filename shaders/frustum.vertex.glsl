#version 110

uniform mat4 mat_proj;
uniform float cx;
uniform float cy;
uniform bool spherical;
attribute vec2 vertex;
varying vec4 fpos;

void main (void)
{
	/* Vertex position in world coordinates: */
	fpos = vec4(vertex, 0.0, 1.0);

	/* Vertex position in screen coordinates: */
	gl_Position = mat_proj * fpos;

	/* Translate fpos to camera position: */
	if (!spherical)
		fpos -= vec4(cx, cy, 0.0, 0.0);
}
