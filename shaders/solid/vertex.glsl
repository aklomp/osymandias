#version 130

uniform mat4 matrix;
in  vec2 vertex;
in  vec4 color;
out vec4 fcolor;

void main (void)
{
	fcolor = color;
	gl_Position = matrix * vec4(vertex, 0.0, 1.0);
}
