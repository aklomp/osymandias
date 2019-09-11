#include "../inlinebin.h"
#include "../programs.h"
#include "spherical.h"

enum	{ CAM
	, MAT_MVP
	, MAT_MV_INV
	, TILE_X
	, TILE_Y
	, TILE_ZOOM
	, VERTEX
	, VP_ANGLE
	, VP_WIDTH
	} ;

static struct input inputs[] = {
	[CAM]        = { .name = "cam",        .type = TYPE_UNIFORM },
	[MAT_MVP]    = { .name = "mat_mvp",    .type = TYPE_UNIFORM },
	[MAT_MV_INV] = { .name = "mat_mv_inv", .type = TYPE_UNIFORM },
	[TILE_X]     = { .name = "tile_x",     .type = TYPE_UNIFORM },
	[TILE_Y]     = { .name = "tile_y",     .type = TYPE_UNIFORM },
	[TILE_ZOOM]  = { .name = "tile_zoom",  .type = TYPE_UNIFORM },
	[VERTEX]     = { .name = "vertex",     .type = TYPE_UNIFORM },
	[VP_ANGLE]   = { .name = "vp_angle",   .type = TYPE_UNIFORM },
	[VP_WIDTH]   = { .name = "vp_width",   .type = TYPE_UNIFORM },
	               { .name = NULL }
};

struct program program_spherical __attribute__((section(".programs"))) =
	{ .name     = "spherical"
	, .vertex   = { .src = SHADER_SPHERICAL_VERTEX   }
	, .fragment = { .src = SHADER_SPHERICAL_FRAGMENT }
	, .inputs   = inputs
	} ;

void
program_spherical_set_tile (const struct cache_node *tile, const struct cache_data *data)
{
	glUniform1i(inputs[TILE_X].loc,    tile->x);
	glUniform1i(inputs[TILE_Y].loc,    tile->y);
	glUniform1i(inputs[TILE_ZOOM].loc, tile->zoom);
	glUniform3fv(inputs[VERTEX].loc, 4, (const float *) &data->coords);
}

void
program_spherical_use (const struct camera *cam, const struct viewport *vp)
{
	glUseProgram(program_spherical.id);
	glUniform3fv(inputs[CAM].loc, 1, vp->cam_pos);
	glUniformMatrix4fv(inputs[MAT_MVP].loc,    1, GL_FALSE, vp->matrix32.modelviewproj);
	glUniformMatrix4fv(inputs[MAT_MV_INV].loc, 1, GL_FALSE, vp->invert32.modelview);
	glUniform1f(inputs[VP_ANGLE].loc, cam->view_angle);
	glUniform1f(inputs[VP_WIDTH].loc, vp->width);
}
