uniform sampler2D tex;
in varying vec2 ftex;

void main (void)
{
	gl_FragColor = texture(tex, ftex);
}
