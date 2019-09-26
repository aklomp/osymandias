#pragma once

#include <stdbool.h>

#include "camera.h"
#include "viewport.h"

#define LAYER(Z)	const struct layer layer_ ## Z __attribute__((section(".layer." # Z)))

struct layer {
	bool (*init)    (const struct viewport *vp);
	void (*paint)   (const struct camera *cam, const struct viewport *vp);
	void (*resize)  (const struct viewport *vp);
	void (*destroy) (void);
};

extern void layers_destroy (void);
extern void layers_paint   (const struct camera *cam, const struct viewport *vp);
extern void layers_resize  (const struct viewport *vp);
extern bool layers_init    (const struct viewport *vp);
