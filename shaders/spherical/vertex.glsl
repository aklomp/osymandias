#version 130

uniform mat4 mat_viewproj;
uniform mat4 mat_model;
uniform mat4 mat_model_inv;
uniform mat4 mat_view_inv;
uniform int  tile_zoom;
uniform vec3 vertex[4];

flat out vec3  tile_origin;
flat out vec3  tile_xaxis;
flat out vec3  tile_yaxis;
flat out vec3  tile_normal;
flat out float tile_xlength0;
flat out float tile_xlength1;
flat out float tile_ylength;

flat   out vec3 cam;
smooth out vec3 p;

void main (void)
{
	// Apply projection matrix to get view coordinates:
	gl_Position = mat_viewproj * mat_model * vec4(vertex[gl_VertexID], 1.0);

	// Zoom level determines screen Z depth,
	// range is -1 (nearest) to 1 (farthest), though the basemap is zero:
	gl_Position.z  = float(tile_zoom) / -20.0 - 0.01;
	gl_Position.z *= gl_Position.w;

	// The camera position is the world space origin, projected back
	// through the view and model matrices:
	cam = (mat_model_inv * mat_view_inv * vec4(vec3(0.0), 1.0)).xyz;

	// Interpolate the position in model space. Do not use the vector point
	// directly, but move it backwards a bit on the camera axis, because
	// the distance to the camera can get very small:
	p = cam + (tile_zoom * vertex[gl_VertexID] - tile_zoom * cam);

	// Get all four vertex coordinates in model space. The tile becomes an
	// isosceles trapezoid. Height is uniform, but width varies by height.
	//
	//    3-+-2
	//   /  |  \
	//  0-------1
	//
	// Choose the reference origin to be in the center top of the
	// parallellogram:
	tile_origin = mix(vertex[3], vertex[2], 0.5);
	tile_xaxis  = normalize(vertex[2] - vertex[3]);

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
