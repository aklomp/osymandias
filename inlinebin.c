#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>

#include "inlinebin.h"

#define DATA_DEF(name)							\
	extern const uint8_t _binary_## name ##_start[];		\
	extern const uint8_t _binary_## name ##_end[];

#define DATA_ELEM(name)							\
	{ .buf = _binary_## name ##_start				\
	, .len = _binary_## name ##_end - _binary_## name ##_start	\
	} ,

#define X_MAP					\
	X (shaders_cursor_vertex_glsl)		\
	X (shaders_cursor_fragment_glsl)	\
	X (shaders_bkgd_vertex_glsl)		\
	X (shaders_bkgd_fragment_glsl)		\
	X (shaders_solid_vertex_glsl)		\
	X (shaders_solid_fragment_glsl)		\
	X (shaders_frustum_vertex_glsl)		\
	X (shaders_frustum_fragment_glsl)

// Define variables pointing to inline data:
#define X(x) DATA_DEF(x)
X_MAP
#undef X

void
inlinebin_get (enum inlinebin member, const void **buf, size_t *len)
{
	// Define an array of inline data descriptors:
	struct {
		const uint8_t	*buf;
		size_t		 len;
	}
	elems[] = {

		// First element is empty sentinel:
		{ .buf = NULL
		, .len = 0
		} ,

		// Populate remaining elements:
		#define X(x) DATA_ELEM(x)
		X_MAP
		#undef X
	};

	// Return requested element to caller:
	*buf = elems[member].buf;
	*len = elems[member].len;
}
