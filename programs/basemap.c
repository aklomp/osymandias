#include <stdbool.h>
#include <GL/gl.h>

#include "../inlinebin.h"
#include "../program.h"
#include "basemap.h"

enum	{ CAM
	, MAT_MV_INV
	, VP_ANGLE
	, VP_HEIGHT
	, VP_WIDTH
	, VERTEX
	} ;

static struct input inputs[] =
	{ [CAM]        = { .name = "cam",        .type = TYPE_UNIFORM }
	, [MAT_MV_INV] = { .name = "mat_mv_inv", .type = TYPE_UNIFORM }
	, [VP_ANGLE]   = { .name = "vp_angle",   .type = TYPE_UNIFORM }
	, [VP_HEIGHT]  = { .name = "vp_height",  .type = TYPE_UNIFORM }
	, [VP_WIDTH]   = { .name = "vp_width",   .type = TYPE_UNIFORM }
	, [VERTEX]     = { .name = "vertex",     .type = TYPE_ATTRIBUTE }
	,                { .name = NULL }
	} ;

static struct program program = {
	.name     = "basemap",
	.vertex   = { SHADER_BASEMAP_VERTEX },
	.fragment = { SHADER_BASEMAP_FRAGMENT },
	.inputs   = inputs,
};

int32_t
program_basemap_loc_vertex (void)
{
	return inputs[VERTEX].loc;
}

void
program_basemap_use (const struct camera *cam, const struct viewport *vp)
{
	glUseProgram(program.id);
	glUniform3fv(inputs[CAM].loc, 1, vp->cam_pos);
	glUniformMatrix4fv(inputs[MAT_MV_INV].loc, 1, GL_FALSE, vp->invert32.modelview);
	glUniform1f(inputs[VP_ANGLE].loc,  cam->view_angle);
	glUniform1f(inputs[VP_HEIGHT].loc, vp->height);
	glUniform1f(inputs[VP_WIDTH].loc,  vp->width);
}

PROGRAM_REGISTER(&program)
