#include <emmintrin.h>

#define vec4f_shuffle(v,a,b,c,d)	((vec4f)_mm_shuffle_epi32((__m128i)v, _MM_SHUFFLE(d, c, b, a)))

static inline vec4f
vec4f_zero (void)
{
	return (vec4f)_mm_setzero_ps();
}

static inline vec4f
vec4f_float (const float f)
{
	return (vec4f)_mm_set1_ps(f);
}

static inline float
vec4f_hsum (const vec4f v)
{
	vec4f t = _mm_add_ps(v, _mm_movehl_ps(v, v));
	vec4f u = _mm_add_ps(t, vec4f_shuffle(t, 1, 1, 1, 1));
	return u[0];
}

// Integer vectors:

static inline vec4i
vec4i_zero (void)
{
	return (vec4i)_mm_setzero_si128();
}

static inline vec4i
vec4i_int (const int i)
{
	return (vec4i)_mm_set1_epi32(i);
}

static inline vec4i
vec4f_to_vec4i (const vec4f v)
{
	return (vec4i)_mm_cvttps_epi32(v);
}
