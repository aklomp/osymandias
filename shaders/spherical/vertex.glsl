#version 130

uniform mat4  mat_mvp_origin;
uniform mat4  mat_mv_inv;
uniform int   tile_zoom;
uniform vec3  cam;
uniform vec3  cam_lowbits;
uniform vec3  vertex[4];
uniform float vp_angle;
uniform float vp_width;

flat out vec3  tile_origin;
flat out vec3  tile_xaxis;
flat out vec3  tile_yaxis;
flat out vec3  tile_normal;
flat out float tile_xlength0;
flat out float tile_xlength1;
flat out float tile_ylength;

smooth out mat4x3 rays;

void main (void)
{
	// Find the vertex position relative to the camera. The result is a
	// direction vector with the camera as the origin, or rather a ray:
	vec3 p = vertex[gl_VertexID] - cam - cam_lowbits;

	// Apply projection matrix to get view coordinates:
	gl_Position = mat_mvp_origin * vec4(p, 1.0);

	// Zoom level determines screen Z depth,
	// range is -1 (nearest) to 1 (farthest), though the basemap is zero:
	gl_Position.z  = float(tile_zoom) / -20.0 - 0.01;
	gl_Position.z *= gl_Position.w;

	// Angle in radians of the arc swept by one pixel:
	float arc = vp_angle / vp_width;

	// Cast four subpixel rays at slight offsets to p for antialiasing:
	//
	//   3  2
	//   0  1
	//
	// Model and view matrices do not involve scaling so the lengths stay
	// the same after transformation. Find the offcenter distance, which is
	// the deviation of the vector over the given angle:
	float offcenter = length(p) * tan(arc / 4.0);

	// Rotate and translate (but do not scale) the deviation lengths:
	rays = mat4x3(mat_mv_inv * mat4(
		vec4(-offcenter,  offcenter, 0.0, 0.0),
		vec4( offcenter,  offcenter, 0.0, 0.0),
		vec4( offcenter, -offcenter, 0.0, 0.0),
		vec4(-offcenter, -offcenter, 0.0, 0.0)));

	// Add the deviation lengths to the base vector:
	rays += mat4x3(p, p, p, p);

	// Get all four vertex coordinates in model space. The tile becomes an
	// isosceles trapezoid. Height is uniform, but width varies by height.
	//
	//    3-+-2
	//   /  |  \
	//  0-------1
	//
	// Choose the reference origin to be in the center top of the
	// parallellogram, make it relative to the camera for more precision:
	tile_xaxis  = normalize(vertex[2] - vertex[3]);
	tile_origin = mix(
		vertex[3] - cam - cam_lowbits,
		vertex[2] - cam - cam_lowbits, 0.5);

	// The Y axis is the axis that goes through sw and is perpendicular to
	// the X axis, so a projection of 3 on 0..1:
	vec3 leftedge = vertex[0] - vertex[3];
	tile_yaxis = leftedge - dot(leftedge, tile_xaxis) * tile_xaxis;

	// The y distance is the length of the unnormalized y axis:
	tile_ylength = length(tile_yaxis);
	tile_yaxis   = normalize(tile_yaxis);

	// Get the lengths of the top and bottom x axes:
	tile_xlength0 = distance(vertex[3], vertex[2]);
	tile_xlength1 = distance(vertex[0], vertex[1]);

	// The normal is the cross product of the axes:
	tile_normal = normalize(cross(tile_xaxis, tile_yaxis));
}
