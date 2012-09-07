uniform float halfwd;
uniform float halfht;

void main(void)
{
	float dist_x = abs(gl_FragCoord.x - halfwd);
	float dist_y = abs(gl_FragCoord.y - halfht);

	if (dist_x > 8.0 || dist_y > 8.0) {
		gl_FragColor = vec4(0.0);
		return;
	}
	if (dist_x < 1.0) {
		gl_FragColor = vec4(vec3(0.0), 0.1 * dist_y);
		return;
	}
	if (dist_y < 1.0) {
		gl_FragColor = vec4(vec3(0.0), 0.1 * dist_x);
		return;
	}
	if (dist_x < 3.0 || dist_y < 3.0) {
		gl_FragColor = vec4(vec3(1.0), 0.4);
		return;
	}
	gl_FragColor = vec4(0.0);
}
