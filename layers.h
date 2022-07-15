#pragma once

#include <stdbool.h>

#include "camera.h"
#include "viewport.h"

extern void layers_destroy (void);
extern void layers_paint   (const struct camera *cam, const struct viewport *vp);
extern void layers_resize  (const struct viewport *vp);
extern bool layers_init    (const struct viewport *vp);
