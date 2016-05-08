#version 130

uniform mat4 mat_viewproj_inv;
uniform mat4 mat_model_inv;

in vec2 vertex;

out vec4 p1;
out vec4 p2;

void main (void)
{
	// The vertex position is always fixed against the screen:
	gl_Position = vec4(vertex, 0.0, 1.0);

	// Unproject two points, at z=0 and z=1:
	p1 = mat_viewproj_inv * vec4(vertex, 0.0, 1.0);
	p2 = mat_viewproj_inv * vec4(vertex, 1.0, 1.0);

	// Undo perspective transform:
	p1 /= vec4(p1.w);
	p2 /= vec4(p2.w);

	// Transform to model space:
	p1 = mat_model_inv * p1;
	p2 = mat_model_inv * p2;

	// p1 and p2 are now two points at the start and end of the frustum,
	// translated into model space. After per-fragment interpolation, they
	// can be thought of as the start and end of the ray through the
	// fragment into model space.
}
