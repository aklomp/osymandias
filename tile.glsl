uniform int offs_x;
uniform int offs_y;
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
	gl_FragColor = texture2D(tex, vec2(float(modx), float(255 - mody)) / 256.0);
}
