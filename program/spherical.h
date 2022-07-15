#pragma once

#include "../cache.h"
#include "../camera.h"
#include "../globe.h"
#include "../viewport.h"

extern void program_spherical_set_tile (const struct cache_node *tile, const struct globe_tile *coords);
extern void program_spherical_use (const struct camera *cam, const struct viewport *vp);
