#include <stdbool.h>
#include <GL/gl.h>

#include "../inlinebin.h"
#include "../programs.h"
#include "solid.h"

static struct input inputs[] =
	{ { .name = "matrix", .type = TYPE_UNIFORM }
	, { .name = "color",  .type = TYPE_ATTRIBUTE }
	, { .name = "vertex", .type = TYPE_ATTRIBUTE }
	, {  NULL }
	} ;

static struct program program =
	{ .name     = "solid"
	, .fragment = { .src = SHADER_SOLID_FRAGMENT }
	, .vertex   = { .src = SHADER_SOLID_VERTEX }
	, .inputs   = inputs
	} ;

struct program *
program_solid (void)
{
	return &program;
}

GLint
program_solid_loc_color (void)
{
	return inputs[1].loc;
}

GLint
program_solid_loc_vertex (void)
{
	return inputs[2].loc;
}

void
program_solid_use (struct program_solid *values)
{
	glUseProgram(program.id);
	glUniformMatrix4fv(inputs[0].loc, 1, GL_FALSE, values->matrix);
}
