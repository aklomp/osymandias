#define vec4f_shuffle(v,a,b,c,d)	((vec4f){ v[a], v[b], v[c], v[d] })

static inline vec4f
vec4f_zero (void)
{
	return (vec4f){ 0.0f, 0.0f, 0.0f, 0.0f };
}

static inline vec4f
vec4f_float (const float f)
{
	return (vec4f){ f, f, f, f };
}

static inline float
vec4f_hsum (const vec4f v)
{
	return v[0] + v[1] + v[2] + v[3];
}

// Integer vectors:

static inline vec4i
vec4i_zero (void)
{
	return (vec4i){ 0, 0, 0, 0 };
}

static inline vec4i
vec4i_int (const int i)
{
	return (vec4i){ i, i, i, i };
}

static inline vec4i
vec4f_to_vec4i (const vec4f v)
{
	return (vec4i){ v[0], v[1], v[2], v[3] };
}
