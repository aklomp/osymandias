#include <stdbool.h>
#include <GL/gl.h>

#include "../inlinebin.h"
#include "../programs.h"
#include "frustum.h"

static struct input inputs[] =
	{ { .name = "mat_proj",    .type = TYPE_UNIFORM }
	, { .name = "mat_frustum", .type = TYPE_UNIFORM }
	, { .name = "mat_model",   .type = TYPE_UNIFORM }
	, { .name = "world_size",  .type = TYPE_UNIFORM }
	, { .name = "spherical",   .type = TYPE_UNIFORM }
	, { .name = "camera",      .type = TYPE_UNIFORM }
	, { .name = "vertex",      .type = TYPE_ATTRIBUTE }
	, { .name = NULL }
	} ;

static struct program program =
	{ .name     = "frustum"
	, .fragment = { .src = SHADER_FRUSTUM_FRAGMENT }
	, .vertex   = { .src = SHADER_FRUSTUM_VERTEX }
	, .inputs   = inputs
	} ;

struct program *
program_frustum (void)
{
	return &program;
}

GLint
program_frustum_loc_vertex (void)
{
	return inputs[6].loc;
}

void
program_frustum_use (struct program_frustum *values)
{
	glUseProgram(program.id);
	glUniformMatrix4fv(inputs[0].loc, 1, GL_FALSE, values->mat_proj);
	glUniformMatrix4fv(inputs[1].loc, 1, GL_FALSE, values->mat_frustum);
	glUniformMatrix4fv(inputs[2].loc, 1, GL_FALSE, values->mat_model);
	glUniform1i(inputs[3].loc, values->world_size);
	glUniform1i(inputs[4].loc, values->spherical);
	glUniform4fv(inputs[5].loc, 1, values->camera);
}
