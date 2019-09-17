#include "../inlinebin.h"
#include "../programs.h"
#include "spherical.h"

enum {
	MAT_VIEWPROJ,
	MAT_MODEL,
	MAT_MODEL_INV,
	MAT_VIEW_INV,
	TILE_X,
	TILE_Y,
	TILE_ZOOM,
	VERTEX,
	VP_ANGLE,
	VP_WIDTH,
};

static struct input inputs[] = {
	[MAT_VIEWPROJ]  = { .name = "mat_viewproj",  .type = TYPE_UNIFORM },
	[MAT_MODEL]     = { .name = "mat_model",     .type = TYPE_UNIFORM },
	[MAT_MODEL_INV] = { .name = "mat_model_inv", .type = TYPE_UNIFORM },
	[MAT_VIEW_INV]  = { .name = "mat_view_inv",  .type = TYPE_UNIFORM },
	[TILE_X]        = { .name = "tile_x",        .type = TYPE_UNIFORM },
	[TILE_Y]        = { .name = "tile_y",        .type = TYPE_UNIFORM },
	[TILE_ZOOM]     = { .name = "tile_zoom",     .type = TYPE_UNIFORM },
	[VERTEX]        = { .name = "vertex",        .type = TYPE_UNIFORM },
	[VP_ANGLE]      = { .name = "vp_angle",      .type = TYPE_UNIFORM },
	[VP_WIDTH]      = { .name = "vp_width",      .type = TYPE_UNIFORM },
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
program_spherical_use (const struct program_spherical *values)
{
	glUseProgram(program_spherical.id);
	glUniformMatrix4fv(inputs[MAT_VIEWPROJ].loc,  1, GL_FALSE, values->mat_viewproj);
	glUniformMatrix4fv(inputs[MAT_MODEL].loc,     1, GL_FALSE, values->mat_model);
	glUniformMatrix4fv(inputs[MAT_MODEL_INV].loc, 1, GL_FALSE, values->mat_model_inv);
	glUniformMatrix4fv(inputs[MAT_VIEW_INV].loc,  1, GL_FALSE, values->mat_view_inv);
	glUniform1f(inputs[VP_ANGLE].loc, values->vp_angle);
	glUniform1f(inputs[VP_WIDTH].loc, values->vp_width);
}
