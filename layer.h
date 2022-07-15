#pragma once

#include <stdbool.h>

#include "camera.h"
#include "layers.h"
#include "viewport.h"

#define LAYER_REGISTER(LAYER)			\
	__attribute__((constructor, used))	\
	static void				\
	link_layer (void)			\
	{					\
		layers_link(LAYER);		\
	}

struct layer {

	// Pointer to the next layer in the linked list.
	struct layer *next;

	// Name of the layer, for display/logging purposes.
	const char *name;

	// Layer's Z depth. Lower numbers are drawn before higher numbers.
	int zdepth;

	// Whether `on_init' has been successfully called on this layer.
	bool created;

	// Layer's current visibility (whether `on_paint' is called).
	bool visible;

	bool (*on_init)    (const struct viewport *vp);
	void (*on_paint)   (const struct camera *cam, const struct viewport *vp);
	void (*on_resize)  (const struct viewport *vp);
	void (*on_destroy) (void);
};
