#include <stdbool.h>
#include <GL/gl.h>

#include "../inlinebin.h"
#include "../programs.h"
#include "frustum.h"

enum	{ MAT_PROJ
	, MAT_VIEWPROJ
	, MAT_MODEL
	, MAT_VIEW_INV
	, MAT_MODEL_INV
	, VERTEX
	} ;

static struct input inputs[] =
	{ [MAT_PROJ]      = { .name = "mat_proj",      .type = TYPE_UNIFORM }
	, [MAT_VIEWPROJ]  = { .name = "mat_viewproj",  .type = TYPE_UNIFORM }
	, [MAT_MODEL]     = { .name = "mat_model",     .type = TYPE_UNIFORM }
	, [MAT_VIEW_INV]  = { .name = "mat_view_inv",  .type = TYPE_UNIFORM }
	, [MAT_MODEL_INV] = { .name = "mat_model_inv", .type = TYPE_UNIFORM }
	, [VERTEX]        = { .name = "vertex",        .type = TYPE_ATTRIBUTE }
	,                   { .name = NULL }
	} ;

struct program program_frustum __attribute__((section(".programs"))) =
	{ .name     = "frustum"
	, .fragment = { .src = SHADER_FRUSTUM_FRAGMENT }
	, .vertex   = { .src = SHADER_FRUSTUM_VERTEX }
	, .inputs   = inputs
	} ;

GLint
program_frustum_loc_vertex (void)
{
	return inputs[VERTEX].loc;
}

void
program_frustum_use (struct program_frustum *values)
{
	glUseProgram(program_frustum.id);
	glUniformMatrix4fv(inputs[MAT_PROJ].loc, 1, GL_FALSE, values->mat_proj);
	glUniformMatrix4fv(inputs[MAT_VIEWPROJ].loc, 1, GL_FALSE, values->mat_viewproj);
	glUniformMatrix4fv(inputs[MAT_MODEL].loc, 1, GL_FALSE, values->mat_model);
	glUniformMatrix4fv(inputs[MAT_VIEW_INV].loc, 1, GL_FALSE, values->mat_view_inv);
	glUniformMatrix4fv(inputs[MAT_MODEL_INV].loc, 1, GL_FALSE, values->mat_model_inv);
}
