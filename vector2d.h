static inline vec4f
vector2d_distance_squared (const float px, const float py, const vec4f vx, const vec4f vy)
{
	// Calculate squared distances from (px, py) to (vx, vy):
	vec4f dx = vx - vec4f_float(px);
	vec4f dy = vy - vec4f_float(py);

	return (dx * dx) + (dy * dy);
}

static inline vec4f
vector2d_distance (const float px, const float py, const vec4f vx, const vec4f vy)
{
	// Calculate distances from (px, py) to (vx, vy):
	return vec4f_sqrt(vector2d_distance_squared(px, py, vx, vy));
}
