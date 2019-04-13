#version 130

uniform mat4 mat_viewproj_inv;
uniform mat4 mat_model_inv;
uniform mat4 mat_view_inv;

in vec2 vertex;

flat          out vec3 cam;
noperspective out vec3 p;

void main (void)
{
	// The vertex position is always fixed against the screen:
	gl_Position = vec4(vertex, 0.0, 1.0);

	// The camera position is the world space origin, projected back
	// through the view and model matrices:
	cam = (mat_model_inv * mat_view_inv * vec4(vec3(0.0), 1.0)).xyz;

	// Unproject a vertex point in NDC space:
	vec4 xp = mat_viewproj_inv * vec4(vertex, 1.0, 1.0);

	// Transform to model space. The camera and p are two points at the
	// start and end of the frustum, translated into model space. After
	// per-fragment interpolation, they can be thought of as the start and
	// end of the ray through the fragment into model space.
	p = (mat_model_inv * (xp / xp.w)).xyz;
}
