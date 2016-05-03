#include <GL/gl.h>
#include "glutil.h"

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
		(void *)(&((struct glutil_vertex *)0)->x));
}

void
glutil_vertex_uv_link (const GLuint loc_xy, const GLuint loc_uv)
{
	// Add pointer to xy attribute:
	glEnableVertexAttribArray(loc_xy);
	glVertexAttribPointer(loc_xy, 2, GL_FLOAT, GL_FALSE,
		sizeof (struct glutil_vertex_uv),
		(void *)(&((struct glutil_vertex_uv *)0)->x));

	// Add pointer to uv attribute:
	glEnableVertexAttribArray(loc_uv);
	glVertexAttribPointer(loc_uv, 2, GL_FLOAT, GL_FALSE,
		sizeof (struct glutil_vertex_uv),
		(void *)(&((struct glutil_vertex_uv *)0)->u));
}

void
glutil_draw_quad (void)
{
	// Draw two triangles which together form one quad:
	glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_BYTE, glutil_index);
}
