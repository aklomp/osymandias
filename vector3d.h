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
