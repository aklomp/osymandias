#include <stdbool.h>
#include <GL/gl.h>

#include "../inlinebin.h"
#include "../programs.h"
#include "frustum.h"

enum	{ MAT_PROJ
	, MAT_FRUSTUM
	, MAT_MODEL
	, WORLD_SIZE
	, SPHERICAL
	, CAMERA
	, VERTEX
	} ;

static struct input inputs[] =
	{ [MAT_PROJ]	= { .name = "mat_proj",    .type = TYPE_UNIFORM }
	, [MAT_FRUSTUM]	= { .name = "mat_frustum", .type = TYPE_UNIFORM }
	, [MAT_MODEL]	= { .name = "mat_model",   .type = TYPE_UNIFORM }
	, [WORLD_SIZE]	= { .name = "world_size",  .type = TYPE_UNIFORM }
	, [SPHERICAL]	= { .name = "spherical",   .type = TYPE_UNIFORM }
	, [CAMERA]	= { .name = "camera",      .type = TYPE_UNIFORM }
	, [VERTEX]	= { .name = "vertex",      .type = TYPE_ATTRIBUTE }
	,		  { .name = NULL }
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
	glUniformMatrix4fv(inputs[MAT_FRUSTUM].loc, 1, GL_FALSE, values->mat_frustum);
	glUniformMatrix4fv(inputs[MAT_MODEL].loc, 1, GL_FALSE, values->mat_model);
	glUniform1i(inputs[WORLD_SIZE].loc, values->world_size);
	glUniform1i(inputs[SPHERICAL].loc, values->spherical);
	glUniform4fv(inputs[CAMERA].loc, 1, values->camera);
}
