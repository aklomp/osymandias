#version 130

uniform sampler2D tex;
in  vec2 ftex;
out vec4 fragcolor;

void main(void)
{
	fragcolor = texture2D(tex, ftex);
}
