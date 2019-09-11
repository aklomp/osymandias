#include <stdbool.h>
#include <GL/gl.h>

#include "../inlinebin.h"
#include "../programs.h"
#include "frustum.h"

enum	{ CAM
	, MAT_MVP_ORIGIN
	, MAT_PROJ
	, VERTEX
	} ;

static struct input inputs[] =
	{ [CAM]            = { .name = "cam",            .type = TYPE_UNIFORM }
	, [MAT_MVP_ORIGIN] = { .name = "mat_mvp_origin", .type = TYPE_UNIFORM }
	, [MAT_PROJ]       = { .name = "mat_proj",       .type = TYPE_UNIFORM }
	, [VERTEX]         = { .name = "vertex",         .type = TYPE_ATTRIBUTE }
	,                    { .name = NULL }
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
	glUniform3fv(inputs[CAM].loc, 1, values->cam);
	glUniformMatrix4fv(inputs[MAT_MVP_ORIGIN].loc, 1, GL_FALSE, values->mat_mvp_origin);
	glUniformMatrix4fv(inputs[MAT_PROJ].loc,       1, GL_FALSE, values->mat_proj);
}
