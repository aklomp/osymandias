#version 130

uniform mat4 mat_proj;
in  vec2 vertex;
in  vec2 texture;
out vec2 ftex;

void main (void)
{
	ftex = texture;
	gl_Position = mat_proj * vec4(vertex, 0.0, 1.0);
}
