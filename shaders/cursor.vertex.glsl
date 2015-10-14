#version 110

uniform mat4 mat_proj;
uniform mat4 mat_view;
attribute vec2 vertex;
varying vec2 fpos;

void main (void)
{
	fpos = vertex;
	gl_Position = mat_proj * mat_view * vec4(vertex, 0.0, 1.0);
}
