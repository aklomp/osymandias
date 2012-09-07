uniform float maxsize;
uniform int offs_x;
uniform int offs_y;

int _mod(int x, int y) {
	return (x - y * (x / y));
}

void diagonal()
{
	if (_mod(int(gl_FragCoord.x - gl_FragCoord.y), 5) == 0) {
		gl_FragColor = vec4(vec3(0.20), 1.0);
	} else {
		gl_FragColor = vec4(vec3(0.12), 1.0);
	}
}

void main(void)
{
	int modx;
	int mody;
	vec4 p;

	/* Get world coordinates: */
	p = gl_ModelViewMatrixInverse * gl_FragCoord;

	/* Check for obvious out-of-range situation: */
	if (p.x < 0.0 || p.y < 0.0 || p.x > maxsize + 5.0 || p.y > maxsize + 5.0) {
		diagonal();
		return;
	}
	/* Is the pixel a tile boundary? */
	modx = _mod(int(gl_FragCoord.x) + offs_x, 256);
	mody = _mod(int(gl_FragCoord.y) + offs_y, 256);

	/* For coords near to the maxsize, check the boundary zone carefully;
	 * the world coordinates lack pixel precision at high zoom levels: */
	if ((p.x > maxsize - 5.0 && modx > 1 && modx < 20)
	 || (p.y > maxsize - 5.0 && mody > 1 && mody < 20)) {
		diagonal();
		return;
	}
	if (modx == 0 || mody == 0) {
		gl_FragColor = vec4(vec3(0.20), 1.0);
	} else {
		gl_FragColor = vec4(vec3(0.15), 1.0);
	}
}
