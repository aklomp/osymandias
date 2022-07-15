#include <stdbool.h>

#include <GL/gl.h>

#include "../glutil.h"
#include "../layer.h"
#include "../program.h"
#include "../program/basemap.h"

static uint32_t vao, vbo;
static const struct glutil_vertex vertex[4] = {
	[0] = { -1.0, -1.0 },
	[1] = {  1.0, -1.0 },
	[2] = {  1.0,  1.0 },
	[3] = { -1.0,  1.0 },
};

static void
on_paint (const struct camera *cam, const struct viewport *vp)
{
	program_basemap_use(cam, vp);
	glBindVertexArray(vao);
	glutil_draw_quad();
	program_none();
}

static void
on_destroy (void)
{
	glDeleteVertexArrays(1, &vao);
	glDeleteBuffers(1, &vbo);
}

static bool
on_init (const struct viewport *vp)
{
	(void) vp;

	// Generate vertex buffer and array objects:
	glGenBuffers(1, &vbo);
	glGenVertexArrays(1, &vao);

	// Bind buffer and vertex array:
	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	glBindVertexArray(vao);

	// Link 'vertex' attribute:
	glutil_vertex_link(program_basemap_loc_vertex());

	// Copy vertices to buffer:
	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertex), vertex, GL_STATIC_DRAW);

	return true;
}

static struct layer layer = {
	.name       = "Basemap",
	.zdepth     = 30,
	.visible    = true,
	.on_init    = &on_init,
	.on_paint   = &on_paint,
	.on_destroy = &on_destroy,
};

LAYER_REGISTER(&layer)
