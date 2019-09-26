#include <stdbool.h>
#include <GL/gl.h>

#include "../inlinebin.h"
#include "../programs.h"
#include "tilepicker.h"

enum	{ CAM
	, MAT_MV_INV
	, VP_ANGLE
	, VP_HEIGHT
	, VP_WIDTH
	, VERTEX
	} ;

static struct input inputs[] =
	{ [CAM]        = { .name = "cam",        .type = TYPE_UNIFORM   }
	, [MAT_MV_INV] = { .name = "mat_mv_inv", .type = TYPE_UNIFORM   }
	, [VP_ANGLE]   = { .name = "vp_angle",   .type = TYPE_UNIFORM   }
	, [VP_HEIGHT]  = { .name = "vp_height",  .type = TYPE_UNIFORM   }
	, [VP_WIDTH]   = { .name = "vp_width",   .type = TYPE_UNIFORM   }
	, [VERTEX]     = { .name = "vertex",     .type = TYPE_ATTRIBUTE }
	,                { .name = NULL }
	} ;

struct program program_tilepicker __attribute__((section(".programs"))) =
	{ .name     = "tilepicker"
	, .vertex   = { .src = SHADER_TILEPICKER_VERTEX   }
	, .fragment = { .src = SHADER_TILEPICKER_FRAGMENT }
	, .inputs   = inputs
	} ;

int
program_tilepicker_loc_vertex (void)
{
	return inputs[VERTEX].loc;
}

void
program_tilepicker_use (const struct program_tilepicker *values)
{
	glUseProgram(program_tilepicker.id);
	glUniform3fv(inputs[CAM].loc, 1, values->cam);
	glUniformMatrix4fv(inputs[MAT_MV_INV].loc, 1, GL_FALSE, values->mat_mv_inv);
	glUniform1f(inputs[VP_ANGLE].loc,  values->vp_angle);
	glUniform1f(inputs[VP_HEIGHT].loc, values->vp_height);
	glUniform1f(inputs[VP_WIDTH].loc,  values->vp_width);
}
