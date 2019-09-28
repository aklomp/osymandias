#include <stdint.h>
#include <stdio.h>
#include <math.h>
#include <GL/gl.h>

#include "camera.h"
#include "glutil.h"
#include "programs.h"
#include "programs/tilepicker.h"
#include "tilepicker.h"
#include "util.h"

#define	IMGSIZE	64

static struct pixel {
	struct tilepicker tile;
	uint32_t valid;
} __attribute__((packed)) imgbuf[IMGSIZE * IMGSIZE];

static GLuint vao;
static GLuint fbo;
static GLuint rbo;
static GLint fb_orig[2];
static GLint vp_orig[4];

static void fbo_bind (void)
{
	// Save current framebuffer bindings to restore them later:
	glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, &fb_orig[0]);
	glGetIntegerv(GL_READ_FRAMEBUFFER_BINDING, &fb_orig[1]);

	// Save original viewport dimensions:
	glGetIntegerv(GL_VIEWPORT, vp_orig);

	// Set new viewport dimensions:
	glViewport(0, 0, IMGSIZE, IMGSIZE);

	// Bind own framebuffer for drawing and reading:
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, fbo);
	glBindFramebuffer(GL_READ_FRAMEBUFFER, fbo);
}

static void fbo_unbind (void)
{
	// Restore original viewport dimensions:
	glViewport(vp_orig[0], vp_orig[1], vp_orig[2], vp_orig[3]);

	// Restore original framebuffer bindings:
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, fb_orig[0]);
	glBindFramebuffer(GL_READ_FRAMEBUFFER, fb_orig[1]);
}

static void
init (void)
{
	static const struct glutil_vertex verts[4] = {
		[0] = { -1.0, -1.0 },
		[1] = {  1.0, -1.0 },
		[2] = {  1.0,  1.0 },
		[3] = { -1.0,  1.0 },
	};

	glGenVertexArrays(1, &vao);
	glGenFramebuffers(1, &fbo);
	glGenRenderbuffers(1, &rbo);

	glBindVertexArray(vao);

	// Map 'vertex' attribute to a member of struct glutil_vertex:
	glutil_vertex_link(program_tilepicker_loc_vertex());

	// Copy vertices to buffer:
	glBufferData(GL_ARRAY_BUFFER, sizeof (verts), verts, GL_STATIC_DRAW);

	// Unbind array:
	glBindVertexArray(0);

	// Create renderbuffer storage and attach to framebuffer:
	fbo_bind();
	glBindRenderbuffer(GL_RENDERBUFFER, rbo);
	glRenderbufferStorage(GL_RENDERBUFFER, GL_RGBA32UI, IMGSIZE, IMGSIZE);
	fbo_unbind();
}

static void
render (const struct viewport *vp, const struct camera *cam)
{
	// Use the tilepicker program:
	program_tilepicker_use(&(struct program_tilepicker) {
		.cam        = vp->cam_pos,
		.mat_mv_inv = vp->invert32.modelview,
		.vp_angle   = cam->view_angle * (IMGSIZE + 2) / IMGSIZE,
		.vp_height  = vp->height,
		.vp_width   = vp->width,
	});

	// Use framebuffer object:
	fbo_bind();
	glFramebufferRenderbuffer(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_RENDERBUFFER, rbo);
	glClear(GL_COLOR_BUFFER_BIT);

	// Draw the quad:
	glBindVertexArray(vao);
	glutil_draw_quad();
	program_none();
	glBindVertexArray(0);

	// Read back the renderbuffer contents:
	glFramebufferRenderbuffer(GL_READ_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_RENDERBUFFER, rbo);
	glReadBuffer(GL_COLOR_ATTACHMENT0);
	glReadPixels(0, 0, IMGSIZE, IMGSIZE, GL_RGBA_INTEGER, GL_UNSIGNED_INT, imgbuf);
	fbo_unbind();
}

// Array with 100 tiles at every zoom level:
static struct bucket {
	size_t used;
	struct tilepicker tile[100];
} bucket[20];

static void
add_tile_to_bucket (const struct tilepicker *tile)
{
	struct bucket *b = &bucket[tile->zoom];

	// Return if bucket is already at capacity:
	if (b->used == NELEM(b->tile))
		return;

	// Check if tile is already in the bucket:
	FOREACH_NELEM (b->tile, b->used, bp)
		if (bp->x == tile->x && bp->y == tile->y)
			return;

	// Add tile to bucket:
	b->tile[b->used++] = *tile;
}

static void
populate_buckets (void)
{
	// Reset buckets:
	FOREACH (bucket, b)
		b->used = 0;

	// Loop over all pixels in imgbuf, add to bucket:
	FOREACH (imgbuf, p)
		if (p->valid)
			add_tile_to_bucket(&p->tile);
}

void
tilepicker_recalc (const struct viewport *vp, const struct camera *cam)
{
	static bool init_done = false;

	// Lazy init:
	if (init_done == false) {
		init();
		init_done = true;
	}

	// Render tile zoom info to buffer:
	render(vp, cam);

	// Populate buckets:
	populate_buckets();
}

static size_t walk_zoom;
static size_t walk_index;

const struct tilepicker *
tilepicker_next (void)
{
	// Move down one zoom level if last element hit:
	while (walk_index == bucket[walk_zoom].used) {

		// We're done if the zoom cannot be downgraded:
		if (walk_zoom == 0)
			return NULL;

		walk_zoom--;
		walk_index = 0;
	}

	return &bucket[walk_zoom].tile[walk_index++];
}

const struct tilepicker *
tilepicker_first (void)
{
	// Walk the list from the highest zoom level to the lowest:
	walk_zoom  = NELEM(bucket) - 1;
	walk_index = 0;
	return tilepicker_next();
}
