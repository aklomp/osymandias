#version 130

uniform mat4 mat_viewproj;
uniform mat4 mat_model;
uniform int  tile_zoom;
uniform int  world_zoom;

in  vec2 vertex;
in  vec2 texture;
out vec2 ftex;

void main (void)
{
	ftex = texture;

	// Convert tile coordinates to world coordinates. Tile coordinates have
	// an origin at top left, world coordinates at bottom left.
	float y = (1 << world_zoom) - vertex.y;

	gl_Position = mat_viewproj * mat_model * vec4(vertex.x, y, 0.0, 1.0);

	// Zoom level determines screen Z depth,
	// range is -1 (nearest) to 1 (farthest), though the basemap is zero:
	gl_Position.z  = float(tile_zoom) / -20.0 - 0.01;
	gl_Position.z *= gl_Position.w;
}
