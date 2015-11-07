// This struct is binary compatible with float[4] and SSE registers:
struct vector {
	float x;
	float y;
	float z;
	float w;
}
__attribute__ ((packed))
__attribute__ ((aligned (16)));

// Define cross-platform vector types:
typedef int32_t vec4i __attribute__ ((vector_size(sizeof(int32_t) * 4))) __attribute__ ((aligned));
typedef float   vec4f __attribute__ ((vector_size(sizeof(float)   * 4))) __attribute__ ((aligned));

// Cast a float[4] or struct vector to vec4f:
#define VEC4F(x)	(*(vec4f *)&(x))

// Include architecture-specific function implementations:
#if defined __SSE2__
	#include "vector/sse2.h"
#else
	#include "vector/plain.h"
#endif

// These functions are architecture-independent:
static inline float
vec_distance_squared (const struct vector *a, const struct vector *b)
{
	const vec4f va = VEC4F(*a);
	const vec4f vb = VEC4F(*b);
	const vec4f dv = va - vb;

	return vec4f_hsum(dv * dv);
}

static inline int
vec4i_hmax (const vec4i i)
{
	int max1 = (i[0] > i[1]) ? i[0] : i[1];
	int max2 = (i[2] > i[3]) ? i[2] : i[3];
	return (max1 > max2) ? max1 : max2;
}
