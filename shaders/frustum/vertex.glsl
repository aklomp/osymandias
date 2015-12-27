#version 130

uniform mat4 mat_proj;
uniform bool spherical;
attribute vec2 vertex;
varying vec4 fpos;

void main (void)
{
	/* Vertex position in world coordinates: */
	fpos = vec4(vertex, 0.0, 1.0);

	/* Vertex position in screen coordinates: */
	gl_Position = mat_proj * fpos;
}
