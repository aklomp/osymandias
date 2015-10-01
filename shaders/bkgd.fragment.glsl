#version 110

uniform sampler2D tex;
varying vec2 ftex;

void main (void)
{
	gl_FragColor = texture2D(tex, ftex);
}
