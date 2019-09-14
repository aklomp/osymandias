#include <string.h>

#include <GL/gl.h>
#include <GL/glu.h>

#include "globe.h"
#include "camera.h"
#include "tilepicker.h"
#include "layers.h"
#include "matrix.h"
#include "programs.h"
#include "viewport.h"

// Screen dimensions:
static struct viewport vp;

void
viewport_destroy (void)
{
	layers_destroy();
	programs_destroy();
}

bool
viewport_unproject (const struct viewport_pos *pos, float *lat, float *lon)
{
	const struct globe *globe = globe_get();
	union vec p1, p2;

	// Ignore if point is outside viewport:
	if (pos->x < 0 || (uint32_t) pos->x >= vp.width)
		return false;

	if (pos->y < 0 || (uint32_t) pos->y >= vp.height)
		return false;

	// Unproject two points at different z index through the
	// view-projection matrix to get two points in world coordinates:
	camera_unproject(&p1, &p2, pos->x, pos->y, &vp);

	// Unproject through the inverse model matrix to get two points in
	// model space:
	mat_vec32_multiply(p1.elem.f, globe->invert.model, p1.elem.f);
	mat_vec32_multiply(p2.elem.f, globe->invert.model, p2.elem.f);

	// Direction vector is difference between points:
	const union vec dir = vec_sub(p1, p2);

	// Intersect these two points with the globe:
	return globe_intersect(&p1, &dir, lat, lon);
}

void
viewport_paint (void)
{
	// Clear the depth buffer:
	glClear(GL_DEPTH_BUFFER_BIT);
	glEnable(GL_DEPTH_TEST);
	glDepthMask(GL_TRUE);

	const struct camera *cam  = camera_get();
	const struct globe *globe = globe_get();

	// Gather and combine view and model matrices:
	if (globe->updated.model) {
		memcpy(vp.matrix64.model, globe->matrix.model, sizeof (vp.matrix64.model));
		memcpy(vp.invert64.model, globe->invert.model, sizeof (vp.invert64.model));

		mat_to_float(vp.matrix32.model, vp.matrix64.model);
		mat_to_float(vp.invert32.model, vp.invert64.model);
	}

	if (cam->updated.view) {
		memcpy(vp.matrix64.view, cam->matrix.view, sizeof (vp.matrix64.view));
		memcpy(vp.invert64.view, cam->invert.view, sizeof (vp.invert64.view));
		memcpy(vp.matrix64.view_origin, cam->matrix.view_origin, sizeof (vp.matrix64.view_origin));

		mat_to_float(vp.matrix32.view, vp.matrix64.view);
		mat_to_float(vp.invert32.view, vp.invert64.view);
	}

	if (cam->updated.proj || cam->updated.view) {
		mat_multiply(vp.matrix64.viewproj, cam->matrix.proj, cam->matrix.view);
		mat_invert(vp.invert64.viewproj, vp.matrix64.viewproj);
		mat_multiply(vp.matrix64.viewproj_origin, cam->matrix.proj, cam->matrix.view_origin);

		mat_to_float(vp.matrix32.viewproj, vp.matrix64.viewproj);
		mat_to_float(vp.invert32.viewproj, vp.invert64.viewproj);
	}

	if (globe->updated.model || cam->updated.proj || cam->updated.view) {
		mat_multiply(vp.invert64.modelview, vp.invert64.model, vp.invert64.view);
		mat_multiply(vp.matrix64.modelviewproj_origin, vp.matrix64.viewproj_origin, vp.matrix64.model);

		mat_to_float(vp.invert32.modelview, vp.invert64.modelview);
		mat_to_float(vp.matrix32.modelviewproj_origin, vp.matrix64.modelviewproj_origin);
	}

	if (globe->updated.model || cam->updated.view) {
		double cam_pos[4], origin[4] = { 0.0, 0.0, 0.0, 1.0 };

		// Find camera position in model space by unprojecting origin:
		mat_vec64_multiply(cam_pos, vp.invert64.modelview, origin);

		for (int i = 0; i < 3; i++) {
			vp.cam_pos[i]         = cam_pos[i];
			vp.cam_pos_lowbits[i] = cam_pos[i] - vp.cam_pos[i];
		}
	}

	if (globe->updated.model || cam->updated.proj || cam->updated.view) {

		// Recalculate the list of visible tiles:
		tilepicker_recalc(&vp, cam);
	}

	// Reset matrix update flags:
	camera_updated_reset();
	globe_updated_reset();

	// Paint all layers:
	layers_paint(camera_get(), &vp);
}

void
viewport_resize (const unsigned int width, const unsigned int height)
{
	vp.width  = width;
	vp.height = height;

	// Setup viewport:
	glViewport(0, 0, vp.width, vp.height);

	// Update camera's projection matrix:
	camera_set_aspect_ratio(&vp);

	// Alert layers:
	layers_resize(&vp);
}

const struct viewport *
viewport_get (void)
{
	return &vp;
}

bool
viewport_init (const uint32_t width, const uint32_t height)
{
	vp.width  = width;
	vp.height = height;

	if (!programs_init())
		return false;

	if (!layers_init(&vp))
		return false;

	if (!camera_init(&vp))
		return false;

	globe_init();
	return true;
}
