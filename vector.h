// NB: maybe explicitly use int32_t here for platforms with 64-bit ints?
typedef int vec4i __attribute__ ((vector_size(sizeof(int) * 4))) __attribute__ ((aligned));
typedef float vec4f __attribute__ ((vector_size(sizeof(float) * 4))) __attribute__ ((aligned));

#ifdef __SSE__
	#include <xmmintrin.h>
	#define vec4f_zero()			(vec4f)_mm_setzero_ps()
	#define vec4f_float(f)			(vec4f)_mm_set1_ps(f)
	#define vec4f_shuffle(v,a,b,c,d)	(vec4f)_mm_shuffle_ps(v, v, _MM_SHUFFLE(d, c, b, a))

	static inline float
	vec4f_hmin (const vec4f v)
	{
		vec4f tmp = _mm_min_ps(_mm_movehl_ps(v, v), v);
		tmp = (vec4f)_mm_min_ps(vec4f_shuffle(tmp, 1, 1, 1, 1), tmp);
		return tmp[0];
	}

	static inline float
	vec4f_hmax (const vec4f v)
	{
		vec4f tmp = _mm_max_ps(_mm_movehl_ps(v, v), v);
		tmp = (vec4f)_mm_max_ps(vec4f_shuffle(tmp, 1, 1, 1, 1), tmp);
		return tmp[0];
	}
#else
	#define vec4f_zero()			(vec4f){ 0.0f, 0.0f, 0.0f, 0.0f }
	#define vec4f_float(f)			(vec4f){ f, f, f, f }
	#define vec4f_shuffle(v,a,b,c,d)	(vec4f){ v[a], v[b], v[c], v[d] }

	static inline float
	vec4f_hmin (const vec4f v)
	{
		float min1 = (v[0] < v[1]) ? v[0] : v[1];
		float min2 = (v[2] < v[3]) ? v[2] : v[3];
		return (min1 < min2) ? min1 : min2;
	}

	static inline float
	vec4f_hmax (const vec4f v)
	{
		float max1 = (v[0] > v[1]) ? v[0] : v[1];
		float max2 = (v[2] > v[3]) ? v[2] : v[3];
		return (max1 > max2) ? max1 : max2;
	}
#endif

#ifdef __SSE2__
	#include <emmintrin.h>
	#define vec4i_zero()			(vec4i)_mm_setzero_si128()
	#define vec4i_int(i)			(vec4i)_mm_set1_epi32(i)
	#define vec4f_to_vec4i(v)		(vec4i)__builtin_ia32_cvttps2dq(v)

	// Can do a single-operand shuffle in SSE2:
	#undef vec4f_shuffle
	#define vec4f_shuffle(v,a,b,c,d)	(vec4f)_mm_shuffle_epi32((__m128i)v, _MM_SHUFFLE(d, c, b, a))
#else
	#define vec4i_zero()			(vec4i){ 0, 0, 0, 0 }
	#define vec4i_int(i)			(vec4i){ i, i, i, i }
	#define vec4f_to_vec4i(v)		(vec4i){ v[0], v[1], v[2], v[3] }
#endif
