#pragma once

#include <stdbool.h>

#include "camera.h"
#include "viewport.h"

#define LAYER(Z) \
	const struct layer layer_ ## Z __attribute__((section(".layer." # Z)))

struct layer {

	// Name of the layer, for display/logging purposes.
	const char *name;

	bool (*on_init)    (const struct viewport *vp);
	void (*on_paint)   (const struct camera *cam, const struct viewport *vp);
	void (*on_resize)  (const struct viewport *vp);
	void (*on_destroy) (void);
};
