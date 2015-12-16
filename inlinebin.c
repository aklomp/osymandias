#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>

#include "inlinebin.h"

#define DATA_DEF(name)							\
	extern const uint8_t _binary_## name ##_start[];			\
	extern const uint8_t _binary_## name ##_end[];

#define DATA_ELEM(name)							\
	{ .buf = _binary_## name ##_start				\
	, .len = _binary_## name ##_end - _binary_## name ##_start	\
	} ,

// Define variables pointing to inline data:
DATA_DEF (shaders_cursor_vertex_glsl)
DATA_DEF (shaders_cursor_fragment_glsl)
DATA_DEF (shaders_bkgd_vertex_glsl)
DATA_DEF (shaders_bkgd_fragment_glsl)
DATA_DEF (shaders_solid_vertex_glsl)
DATA_DEF (shaders_solid_fragment_glsl)
DATA_DEF (shaders_frustum_vertex_glsl)
DATA_DEF (shaders_frustum_fragment_glsl)

void
inlinebin_get (enum inlinebin member, const void **buf, size_t *len)
{
	// Define an array of inline data descriptors:
	struct {
		const uint8_t	*buf;
		size_t		 len;
	}
	elems[] = {
		{ .buf = NULL
		, .len = 0
		} ,
		DATA_ELEM (shaders_cursor_vertex_glsl)
		DATA_ELEM (shaders_cursor_fragment_glsl)
		DATA_ELEM (shaders_bkgd_vertex_glsl)
		DATA_ELEM (shaders_bkgd_fragment_glsl)
		DATA_ELEM (shaders_solid_vertex_glsl)
		DATA_ELEM (shaders_solid_fragment_glsl)
		DATA_ELEM (shaders_frustum_vertex_glsl)
		DATA_ELEM (shaders_frustum_fragment_glsl)
	};

	// Return requested element to caller:
	*buf = elems[member].buf;
	*len = elems[member].len;
}
