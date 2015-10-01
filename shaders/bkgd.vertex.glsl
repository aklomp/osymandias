#version 110

attribute vec3 vertex;
attribute vec3 texture;
varying vec2 ftex;

void main (void)
{
	ftex = texture.xy;
	gl_Position = vec4(vertex, 1.0);
}
