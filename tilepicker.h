#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "camera.h"
#include "viewport.h"

struct tilepicker {
	uint32_t x;
	uint32_t y;
	uint32_t zoom;
} __attribute__((packed));

extern void tilepicker_recalc (const struct viewport *vp, const struct camera *cam);
extern const struct tilepicker *tilepicker_first (void);
extern const struct tilepicker *tilepicker_next  (void);
