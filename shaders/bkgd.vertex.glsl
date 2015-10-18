#version 130

attribute vec2 vertex;
attribute vec2 texture;
varying vec2 ftex;

void main (void)
{
	ftex = texture;
	gl_Position = vec4(vertex, 0.5, 1.0);
}
