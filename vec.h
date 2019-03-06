// Some generic utility functions for 'union vec' that are not found in the
// source library.

#pragma once

#include <vec/vec.h>

// Return square of input vector (a * a):
static inline union vec
vec_square (const union vec a)
{
	return vec_mul(a, a);
}

// Add three vectors together:
static inline union vec
vec_add3 (const union vec a, const union vec b, const union vec c)
{
	return vec_add(vec_add(a, b), c);
}

// Return negated vector (-a):
static inline union vec
vec_negate (const union vec a)
{
	return vec_sub(vec_zero(), a);
}

// Return integer vector element with highest value:
static inline int32_t
vec_imax (const union vec a)
{
	const int32_t max1 = a.elem.i[0] > a.elem.i[1] ? a.elem.i[0] : a.elem.i[1];
	const int32_t max2 = a.elem.i[2] > a.elem.i[3] ? a.elem.i[2] : a.elem.i[3];

	return max1 > max2 ? max1 : max2;
}

// Return integer vector element with highest value:
static inline int32_t
vec_imin (const union vec a)
{
	const int32_t min1 = a.elem.i[0] < a.elem.i[1] ? a.elem.i[0] : a.elem.i[1];
	const int32_t min2 = a.elem.i[2] < a.elem.i[3] ? a.elem.i[2] : a.elem.i[3];

	return min1 < min2 ? min1 : min2;
}

// Return squared distance between two 3D vectors:
// ((bx - ax)^2 + (by - ay)^2 + (bz - az)^2)
static inline float
vec_distance_squared (const union vec a, const union vec b)
{
	// Difference vector (b - a):
	const union vec diff = vec_sub(b, a);

	// Square the difference:
	const union vec squared = vec_square(diff);

	// Add x, y and z components:
	return squared.x + squared.y + squared.z;
}

// Return shuffled vector:
static inline union vec
vec_shuffle (const union vec a, const int p0, const int p1, const int p2, const int p3)
{
	return vec(a.elem.f[p0], a.elem.f[p1], a.elem.f[p2], a.elem.f[p3]);
}
