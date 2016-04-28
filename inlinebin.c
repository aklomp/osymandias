#include <stddef.h>

#include "inlinebin.h"

#define DATA_DEF(name)							\
	extern const char _binary_## name ##_start[];			\
	extern const char _binary_## name ##_end[];

#define DATA_ELEM(id, name)						\
	[id] = {							\
		.buf = _binary_## name ##_start,			\
		.len = _binary_## name ##_end				\
		     - _binary_## name ##_start,			\
	},

#define X_MAP								\
	X (SHADER_CURSOR_VERTEX,	shaders_cursor_vertex_glsl)	\
	X (SHADER_CURSOR_FRAGMENT,	shaders_cursor_fragment_glsl)	\
	X (SHADER_BKGD_VERTEX,		shaders_bkgd_vertex_glsl)	\
	X (SHADER_BKGD_FRAGMENT,	shaders_bkgd_fragment_glsl)	\
	X (SHADER_SOLID_VERTEX,		shaders_solid_vertex_glsl)	\
	X (SHADER_SOLID_FRAGMENT,	shaders_solid_fragment_glsl)	\
	X (SHADER_FRUSTUM_VERTEX,	shaders_frustum_vertex_glsl)	\
	X (SHADER_FRUSTUM_FRAGMENT,	shaders_frustum_fragment_glsl)	\
	X (TEXTURE_BACKGROUND,		textures_background_png)	\
	X (TEXTURE_CURSOR,		textures_cursor_png)		\

// Define variables pointing to inline data:
#define X(id, name) DATA_DEF(name)
X_MAP
#undef X

void
inlinebin_get (enum inlinebin member, const char **buf, size_t *len)
{
	// Define an array of inline data descriptors:
	struct {
		const char	*buf;
		size_t		 len;
	}
	elems[] = {

		// First element is empty sentinel:
		[INLINEBIN_NONE] = {
			.buf = NULL,
			.len = 0,
		},

		// Populate remaining elements:
		#define X(id, name) DATA_ELEM(id, name)
		X_MAP
		#undef X
	};

	// Return requested element to caller:
	*buf = elems[member].buf;
	*len = elems[member].len;
}
