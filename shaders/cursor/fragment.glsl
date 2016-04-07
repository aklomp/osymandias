#version 130

varying vec2 fpos;
out vec4 fragcolor;

void main(void)
{
	vec2 dist = abs(fpos);

	if (dist.x > 8.0 || dist.y > 8.0) {
		fragcolor = vec4(0.0);
		return;
	}
	if (dist.x < 1.0) {
		fragcolor = vec4(vec3(0.0), 0.1 * dist.y);
		return;
	}
	if (dist.y < 1.0) {
		fragcolor = vec4(vec3(0.0), 0.1 * dist.x);
		return;
	}
	if (dist.x < 3.0 || dist.y < 3.0) {
		fragcolor = vec4(vec3(1.0), 0.4);
		return;
	}
	discard;
}
