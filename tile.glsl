uniform int offs_x;
uniform int offs_y;
uniform int texture_offs_x;
uniform int texture_offs_y;
uniform float zoomfactor;
uniform sampler2D tex;

int _mod(int x, int y) {
	return (x - y * (x / y));
}

void main()
{
	int modx = _mod(int(gl_FragCoord.x) + offs_x, 256);
	int mody = _mod(int(gl_FragCoord.y) + offs_y, 256);

	if (gl_TexCoord[0].s > 0.9) {
		if (modx < 20) { gl_FragColor = vec4(0.0); return; }
	} else if (gl_TexCoord[0].s < 0.1) {
		if (modx > 240) { gl_FragColor = vec4(0.0); return; }
	}
	if (gl_TexCoord[0].t > 0.9) {
		if (mody < 20) { gl_FragColor = vec4(0.0); return; }
	} else if (gl_TexCoord[0].t < 0.1) {
		if (mody > 240) { gl_FragColor = vec4(0.0); return; }
	}
	/* A zoomfactor of 256.0 is 1:1, 512.0 is 2:1, and so on: */
	gl_FragColor = texture2D(tex,
		  vec2(float(texture_offs_x), float(texture_offs_y)) / 256.0
		+ vec2(float(modx), float(255 - mody)) / zoomfactor
	);
}
