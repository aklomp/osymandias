#pragma once

#include <stdint.h>

#include "../camera.h"
#include "../viewport.h"

extern int32_t program_basemap_loc_vertex (void);
extern void    program_basemap_use (const struct camera *cam, const struct viewport *vp);
