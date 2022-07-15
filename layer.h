#pragma once

#include <stdbool.h>

#include "camera.h"
#include "viewport.h"

#define LAYER(Z) \
	const struct layer layer_ ## Z __attribute__((section(".layer." # Z)))

struct layer {
	bool (*init)    (const struct viewport *vp);
	void (*paint)   (const struct camera *cam, const struct viewport *vp);
	void (*resize)  (const struct viewport *vp);
	void (*destroy) (void);
};
