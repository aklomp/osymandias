#version 130

uniform mat4  mat_mv_inv;
uniform float vp_angle;		// Horizontal viewing angle in radians
uniform float vp_height;	// Viewport height in pixels
uniform float vp_width;		// Viewport width in pixels
uniform vec3  cam;

in vec2 vertex;

noperspective out vec3  p;
smooth        out float frag_look_angle;
flat          out float frag_arc_angle;

void main (void)
{
	// The vertex position is always fixed against the screen:
	gl_Position = vec4(vertex, 0.0, 1.0);

	// Half of the total horizontal view angle, the deviation from center:
	float alpha = vp_angle / 2.0;

	// Use trigonometry to place the vertex point in world space. The
	// camera is a point at the start of the frustum, while p is a ray
	// (relative to the camera) that points to the end of the frustum.
	// After per-fragment interpolation, they can be thought of as the
	// origin and direction of a ray through the fragment into model space.
	vec3 vpoint = vec3(vertex * sin(alpha) * vec2(1.0, vp_height / vp_width), -cos(alpha));
	p = (mat_mv_inv * vec4(vpoint, 1.0)).xyz - cam;

	// Calculate the horizontal angle between the camera's lookat vector
	// and the direction to the fragment, and interpolate per fragment:
	frag_look_angle = alpha * vertex.x;

	// Calculate the angle of the arc swept by one window pixel:
	frag_arc_angle = vp_angle / vp_width;
}
