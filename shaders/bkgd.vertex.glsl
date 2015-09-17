in varying vec3 vertex;
in varying vec3 texture;

out varying vec2 ftex;

void main (void)
{
	ftex = texture;
	gl_Position = vec4(vertex, 1.0);
}
