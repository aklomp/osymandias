#version 130

uniform mat4 mat_viewproj_inv;
uniform mat4 mat_model_inv;

in vec2 vertex;

noperspective out vec3 p1;
noperspective out vec3 p2;

void main (void)
{
	// The vertex position is always fixed against the screen:
	gl_Position = vec4(vertex, 0.0, 1.0);

	// Unproject two points, at z=0 and z=1:
	vec4 xp1 = mat_viewproj_inv * vec4(vertex, 0.0, 1.0);
	vec4 xp2 = mat_viewproj_inv * vec4(vertex, 1.0, 1.0);

	// Undo perspective transform:
	xp1 /= xp1.w;
	xp2 /= xp2.w;

	// Transform to model space:
	xp1 = mat_model_inv * xp1;
	xp2 = mat_model_inv * xp2;

	// p1 and p2 are two points at the start and end of the frustum,
	// translated into model space. After per-fragment interpolation, they
	// can be thought of as the start and end of the ray through the
	// fragment into model space.
	p1 = xp1.xyz;
	p2 = xp2.xyz;
}
