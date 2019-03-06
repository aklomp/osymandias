#include <stdlib.h>

#include "glutil.h"
#include "png.h"

// Array of indices. If we have a quad defined by these corners:
//
//   3--2
//   |  |
//   0--1
//
// we define two counterclockwise triangles, 0-2-3 and 0-1-2:
static GLubyte glutil_index[6] = {
	0, 2, 3,
	0, 1, 2,
};

void
glutil_vertex_link (const GLuint loc_xy)
{
	// Add pointer to xy attribute:
	glEnableVertexAttribArray(loc_xy);
	glVertexAttribPointer(loc_xy, 2, GL_FLOAT, GL_FALSE,
		sizeof (struct glutil_vertex),
		(void *) offsetof(struct glutil_vertex, x));
}

void
glutil_vertex_uv_link (const GLuint loc_xy, const GLuint loc_uv)
{
	// Add pointer to xy attribute:
	glEnableVertexAttribArray(loc_xy);
	glVertexAttribPointer(loc_xy, 2, GL_FLOAT, GL_FALSE,
		sizeof (struct glutil_vertex_uv),
		(void *) offsetof(struct glutil_vertex_uv, x));

	// Add pointer to uv attribute:
	glEnableVertexAttribArray(loc_uv);
	glVertexAttribPointer(loc_uv, 2, GL_FLOAT, GL_FALSE,
		sizeof (struct glutil_vertex_uv),
		(void *) offsetof(struct glutil_vertex_uv, u));
}

void
glutil_draw_quad (void)
{
	// Draw two triangles which together form one quad:
	glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_BYTE, glutil_index);
}

bool
glutil_texture_load (struct glutil_texture *tex)
{
	char		*raw;
	const char	*png;
	size_t		 len;

	// Get inline texture as PNG blob:
	inlinebin_get(tex->src, &png, &len);

	// Decode PNG to raw format:
	if (png_load(png, len, &tex->height, &tex->width, &raw) == false)
		return false;

	// Generate texture:
	glGenTextures(1, &tex->id);
	glBindTexture(GL_TEXTURE_2D, tex->id);

	// Default texture settings:
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

	// Upload raw texture data:
	glTexImage2D	( GL_TEXTURE_2D		// Target
			, 0			// Mipmap level
			, tex->type		// Internal format
			, tex->width		// Width
			, tex->height		// height
			, 0			// Border, must be 0
			, tex->type		// Format
			, GL_UNSIGNED_BYTE	// Data type
			, raw			// Raw data
			) ;

	// Free memory:
	free(raw);

	return true;
}
