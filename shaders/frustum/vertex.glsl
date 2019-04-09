#version 130

uniform mat4 mat_model_inv;
uniform mat4 mat_view_inv;
uniform mat4 mat_proj;

in vec2 vertex;

smooth out vec4 fpos;
flat   out vec3 cam;

void main (void)
{
	// Vertex position in world coordinates:
	fpos = vec4(vertex, 0.0, 1.0);

	// Vertex position in screen coordinates:
	gl_Position = mat_proj * fpos;

	// The camera position is the world space origin, projected back
	// through the view and model matrices:
	cam = (mat_model_inv * mat_view_inv * vec4(vec3(0.0), 1.0)).xyz;
}
