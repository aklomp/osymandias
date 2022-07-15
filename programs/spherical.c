#include "../inlinebin.h"
#include "../program.h"
#include "spherical.h"

enum	{ CAM
	, CAM_LOWBITS
	, MAT_MVP_ORIGIN
	, MAT_MV_INV
	, TILE_X
	, TILE_Y
	, TILE_ZOOM
	, VERTEX
	, VP_ANGLE
	, VP_WIDTH
	} ;

static struct input inputs[] = {
	[CAM]            = { .name = "cam",            .type = TYPE_UNIFORM },
	[CAM_LOWBITS]    = { .name = "cam_lowbits",    .type = TYPE_UNIFORM },
	[MAT_MVP_ORIGIN] = { .name = "mat_mvp_origin", .type = TYPE_UNIFORM },
	[MAT_MV_INV]     = { .name = "mat_mv_inv",     .type = TYPE_UNIFORM },
	[TILE_X]         = { .name = "tile_x",         .type = TYPE_UNIFORM },
	[TILE_Y]         = { .name = "tile_y",         .type = TYPE_UNIFORM },
	[TILE_ZOOM]      = { .name = "tile_zoom",      .type = TYPE_UNIFORM },
	[VERTEX]         = { .name = "vertex",         .type = TYPE_UNIFORM },
	[VP_ANGLE]       = { .name = "vp_angle",       .type = TYPE_UNIFORM },
	[VP_WIDTH]       = { .name = "vp_width",       .type = TYPE_UNIFORM },
	                   { .name = NULL }
};

static struct program program = {
	.name     = "spherical",
	.vertex   = { SHADER_SPHERICAL_VERTEX   },
	.fragment = { SHADER_SPHERICAL_FRAGMENT },
	.inputs   = inputs,
};

void
program_spherical_set_tile (const struct cache_node *tile, const struct globe_tile *coords)
{
	glUniform1i(inputs[TILE_X].loc,    tile->x);
	glUniform1i(inputs[TILE_Y].loc,    tile->y);
	glUniform1i(inputs[TILE_ZOOM].loc, tile->zoom);
	glUniform3fv(inputs[VERTEX].loc, 4, (const float *) coords);
}

void
program_spherical_use (const struct camera *cam, const struct viewport *vp)
{
	glUseProgram(program.id);
	glUniform3fv(inputs[CAM].loc,         1, vp->cam_pos);
	glUniform3fv(inputs[CAM_LOWBITS].loc, 1, vp->cam_pos_lowbits);
	glUniformMatrix4fv(inputs[MAT_MVP_ORIGIN].loc, 1, GL_FALSE, vp->matrix32.modelviewproj_origin);
	glUniformMatrix4fv(inputs[MAT_MV_INV].loc,     1, GL_FALSE, vp->invert32.modelview);
	glUniform1f(inputs[VP_ANGLE].loc, cam->view_angle);
	glUniform1f(inputs[VP_WIDTH].loc, vp->width);
}

PROGRAM_REGISTER(&program)
