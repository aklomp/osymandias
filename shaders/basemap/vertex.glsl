#version 130

uniform mat4  mat_viewproj_inv;
uniform mat4  mat_model_inv;
uniform mat4  mat_view_inv;
uniform float vp_angle;		// Horizontal viewing angle in radians
uniform float vp_width;		// Viewport width in pixels

in vec2 vertex;

flat          out vec3  cam;
noperspective out vec3  p;
smooth        out float frag_look_angle;
flat          out float frag_arc_angle;

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

	// Calculate the horizontal angle between the camera's lookat vector
	// and the direction to the fragment, and interpolate per fragment:
	frag_look_angle = vp_angle * vertex.x / 2.0;

	// Calculate the angle of the arc swept by one window pixel:
	frag_arc_angle = vp_angle / vp_width;
}
