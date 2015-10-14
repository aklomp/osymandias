#version 110

varying vec2 fpos;

void main(void)
{
	vec2 dist = abs(fpos);

	if (dist.x > 8.0 || dist.y > 8.0) {
		gl_FragColor = vec4(0.0);
		return;
	}
	if (dist.x < 1.0) {
		gl_FragColor = vec4(vec3(0.0), 0.1 * dist.y);
		return;
	}
	if (dist.y < 1.0) {
		gl_FragColor = vec4(vec3(0.0), 0.1 * dist.x);
		return;
	}
	if (dist.x < 3.0 || dist.y < 3.0) {
		gl_FragColor = vec4(vec3(1.0), 0.4);
		return;
	}
	gl_FragColor = vec4(0.0);
}
