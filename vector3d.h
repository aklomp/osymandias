static inline vec4f
vector3d_cross (const vec4f a, const vec4f b)
{
#ifdef __SSE__
	vec4f tmp = _mm_sub_ps(
		_mm_mul_ps(a, vec4f_shuffle(b, 1, 2, 0, 3)),
		_mm_mul_ps(b, vec4f_shuffle(a, 1, 2, 0, 3))
	);
	return vec4f_shuffle(tmp, 1, 2, 0, 3);
#else
	vec4f x = {
		a[1] * b[2],
		a[2] * b[0],
		a[0] * b[1],
		0.0f
	};
	vec4f y = {
		a[2] * b[1],
		a[0] * b[2],
		a[1] * b[0],
		0.0f
	};
	return x - y;
#endif
}

static inline vec4f
plane_from_normal_and_point (const vec4f n, const vec4f p)
{
	// Create a plane { a, b, c, d } from a normal and a point.
	// Normal and point have the form { x, y, z, 0 }.
	//
	// a, b and c are simply copied from the normal.
	// d is calculated by knowing that ax + by + cz + d = 0,
	// so d = 0 - ax - by - cz:
	return (vec4f){ n[0], n[1], n[2], -vec4f_hsum(p * n) };
}

static inline bool
point_in_front_of_plane (const vec4f point, const vec4f plane)
{
	// The sign of the distance between point and plane.
	// Point is { x, y, z, ? }, plane is { a, b, c, d }.
	return (vec4f_hsum((vec4f){ point[0], point[1], point[2], 1 } * plane) > 0);
}
