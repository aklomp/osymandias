#include <stdbool.h>
#include <GL/gl.h>

#include "../inlinebin.h"
#include "../programs.h"
#include "basemap.h"

enum	{ MAT_VIEWPROJ_INV
	, MAT_MODEL_INV
	, MAT_VIEW_INV
	, VERTEX
	} ;

static struct input inputs[] =
	{ [MAT_VIEWPROJ_INV] = { .name = "mat_viewproj_inv", .type = TYPE_UNIFORM }
	, [MAT_MODEL_INV]    = { .name = "mat_model_inv",    .type = TYPE_UNIFORM }
	, [MAT_VIEW_INV]     = { .name = "mat_view_inv",     .type = TYPE_UNIFORM }
	, [VERTEX]           = { .name = "vertex",           .type = TYPE_ATTRIBUTE }
	,                      { .name = NULL }
	} ;

struct program program_basemap __attribute__((section(".programs"))) =
	{ .name     = "basemap"
	, .vertex   = { .src = SHADER_BASEMAP_VERTEX }
	, .fragment = { .src = SHADER_BASEMAP_FRAGMENT }
	, .inputs   = inputs
	} ;

int32_t
program_basemap_loc_vertex (void)
{
	return inputs[VERTEX].loc;
}

void
program_basemap_use (const struct program_basemap *values)
{
	glUseProgram(program_basemap.id);
	glUniformMatrix4fv(inputs[MAT_VIEWPROJ_INV].loc, 1, GL_FALSE, values->mat_viewproj_inv);
	glUniformMatrix4fv(inputs[MAT_MODEL_INV].loc, 1, GL_FALSE, values->mat_model_inv);
	glUniformMatrix4fv(inputs[MAT_VIEW_INV].loc, 1, GL_FALSE, values->mat_view_inv);
}
