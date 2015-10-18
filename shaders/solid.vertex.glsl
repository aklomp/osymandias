#version 130

uniform mat4 matrix;
attribute vec2 vertex;
attribute vec4 color;
varying vec4 fcolor;

void main (void)
{
	fcolor = color;
	gl_Position = matrix * vec4(vertex, 0.0, 1.0);
}
