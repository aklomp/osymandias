// This struct is binary compatible with float[4] and SSE registers:
struct vector {
	float x;
	float y;
	float z;
	float w;
}
__attribute__ ((packed))
__attribute__ ((aligned (16)));

typedef int16_t vec8i __attribute__ ((vector_size(sizeof(int16_t) * 8))) __attribute__ ((aligned));
typedef int32_t vec4i __attribute__ ((vector_size(sizeof(int32_t) * 4))) __attribute__ ((aligned));
typedef float vec4f __attribute__ ((vector_size(sizeof(float) * 4))) __attribute__ ((aligned));

// Cast a float[4] or struct vector to vec4f:
#define VEC4F(x)	(*(vec4f *)&(x))

#if defined __SSE3__
	#include <pmmintrin.h>
#elif defined __SSE2__
	#include <emmintrin.h>
#elif defined __SSE__
	#include <xmmintrin.h>
#endif

// vec4f_shuffle can't be a static function because the mask should be constant
// at compile time. GCC will correctly compile a static function with constant
// parameters to a constant mask, but clang (and thus scan-build) complains: */
#if defined __SSE2__
	#define vec4f_shuffle(v,a,b,c,d)	((vec4f)_mm_shuffle_epi32((__m128i)v, _MM_SHUFFLE(d, c, b, a)))
#elif defined __SSE__
	#define vec4f_shuffle(v,a,b,c,d)	((vec4f)_mm_shuffle_ps(v, v, _MM_SHUFFLE(d, c, b, a)))
#else
	#define vec4f_shuffle(v,a,b,c,d)	((vec4f){ v[a], v[b], v[c], v[d] })
#endif

static inline vec4f
vec4f_zero (void)
{
#ifdef __SSE__
	return (vec4f)_mm_setzero_ps();
#else
	return (vec4f){ 0.0f, 0.0f, 0.0f, 0.0f };
#endif
}

static inline vec4f
vec4f_float (const float f)
{
#ifdef __SSE__
	return (vec4f)_mm_set1_ps(f);
#else
	return (vec4f){ f, f, f, f };
#endif
}

static inline vec4f
vec4f_sqrt (const vec4f v)
{
#ifdef __SSE__
	return (vec4f)_mm_sqrt_ps(v);
#else
	return (vec4f){ sqrtf(v[0]), sqrtf(v[1]), sqrtf(v[2]), sqrtf(v[3]) };
#endif
}

static inline float
vec4f_hmin (const vec4f v)
{
#ifdef __SSE__
	vec4f tmp = _mm_min_ps(_mm_movehl_ps(v, v), v);
	tmp = (vec4f)_mm_min_ps(vec4f_shuffle(tmp, 1, 1, 1, 1), tmp);
	return tmp[0];
#else
	return fminf(fminf(v[0], v[1]), fminf(v[2], v[3]));
#endif
}

static inline float
vec4f_hmax (const vec4f v)
{
#ifdef __SSE__
	vec4f tmp = _mm_max_ps(_mm_movehl_ps(v, v), v);
	tmp = (vec4f)_mm_max_ps(vec4f_shuffle(tmp, 1, 1, 1, 1), tmp);
	return tmp[0];
#else
	return fmaxf(fmaxf(v[0], v[1]), fmaxf(v[2], v[3]));
#endif
}

static inline float
vec4f_hsum (const vec4f v)
{
#if defined __SSE3__
	vec4f u = _mm_hadd_ps(v, v);
	u = _mm_hadd_ps(u, u);
	return u[0];
#elif defined __SSE__
	vec4f t = _mm_add_ps(v, _mm_movehl_ps(v, v));
	vec4f u = _mm_add_ps(t, vec4f_shuffle(t, 1, 1, 1, 1));
	return u[0];
#else
	return v[0] + v[1] + v[2] + v[3];
#endif
}

static inline float
vec_distance_squared (const struct vector *a, const struct vector *b)
{
	vec4f va = VEC4F(*a);
	vec4f vb = VEC4F(*b);
	vec4f dv = va - vb;

	return vec4f_hsum(dv * dv);
}

// Integer vectors:

static inline vec4i
vec4i_zero (void)
{
#ifdef __SSE2__
	return (vec4i)_mm_setzero_si128();
#else
	return (vec4i){ 0, 0, 0, 0 };
#endif
}

static inline vec4i
vec4i_int (const int i)
{
#ifdef __SSE2__
	return (vec4i)_mm_set1_epi32(i);
#else
	return (vec4i){ i, i, i, i };
#endif
}

static inline int
vec4i_hmax (const vec4i i)
{
#if defined __SSE4__
	vec4i tmp = _mm_max_epi32(_mm_movehl_ps(v, v), v);
	tmp = (vec4i)_mm_max_epi32(vec4f_shuffle((vec4f)tmp, 1, 1, 1, 1), (vec4i)tmp);
	return tmp[0];
#else
	int max1 = (i[0] > i[1]) ? i[0] : i[1];
	int max2 = (i[2] > i[3]) ? i[2] : i[3];
	return (max1 > max2) ? max1 : max2;
#endif
}

static inline vec4i
vec4f_to_vec4i (const vec4f v)
{
#ifdef __SSE2__
	return (vec4i)_mm_cvttps_epi32(v);
#else
	return (vec4i){ v[0], v[1], v[2], v[3] };
#endif
}

static inline bool
vec4i_all_false (const vec4i v)
{
	// Benchmarked to generate faster code than our SSE equivalent:
	return (!v[0] && !v[1] && !v[2] && !v[3]);
}

static inline bool
vec4i_all_same (const vec4i v)
{
	// Benchmarked to generate faster code than our SSE equivalent:
	return (v[0] == v[1] && v[1] == v[2] && v[2] == v[3]);
}

// Signed char vectors:

static inline vec8i
vec8i_from_vec4i (const vec4i a, const vec4i b)
{
#ifdef __SSE2__
	return (vec8i)_mm_packs_epi16((__m128i)a, (__m128i)b);
#else
	return (vec8i){
		(int16_t)a[0], (int16_t)a[1], (int16_t)a[2], (int16_t)a[3],
		(int16_t)b[0], (int16_t)b[1], (int16_t)b[2], (int16_t)b[3]
	};
#endif
}

static inline vec8i
vec8i_int (const int i)
{
#ifdef __SSE2__
	return (vec8i)_mm_set1_epi16(i);
#else
	return (vec8i){
		(int16_t)i, (int16_t)i, (int16_t)i, (int16_t)i,
		(int16_t)i, (int16_t)i, (int16_t)i, (int16_t)i
	};
#endif
}

static inline vec8i
vec8i_zero (void)
{
	return (vec8i)vec4f_zero();
}

static inline bool
vec8i_all_zero (const vec8i v)
{
#if defined __SSE4__
	return _mm_testz_si128((__m128)v, (__m128)v);
#elif defined __SSE__
	return (_mm_movemask_ps(_mm_cmpeq_ps((__m128)v, _mm_setzero_ps())) == 0xF);
#else
	vec4i w = (vec4i)v;
	return (w[0] == 0 && w[1] == 0 && w[2] == 0 && w[3] == 0);
#endif
}
