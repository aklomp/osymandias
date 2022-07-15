#include <stdbool.h>
#include <GL/gl.h>

#include "../inlinebin.h"
#include "../program.h"
#include "solid.h"

enum	{ MATRIX
	, COLOR
	, VERTEX
	} ;

static struct input inputs[] =
	{ [MATRIX]	= { .name = "matrix", .type = TYPE_UNIFORM }
	, [COLOR]	= { .name = "color",  .type = TYPE_ATTRIBUTE }
	, [VERTEX]	= { .name = "vertex", .type = TYPE_ATTRIBUTE }
	,		  { .name = NULL }
	} ;

static struct program program = {
	.name     = "solid",
	.vertex   = { SHADER_SOLID_VERTEX },
	.fragment = { SHADER_SOLID_FRAGMENT },
	.inputs   = inputs,
};

GLint
program_solid_loc_color (void)
{
	return inputs[COLOR].loc;
}

GLint
program_solid_loc_vertex (void)
{
	return inputs[VERTEX].loc;
}

void
program_solid_use (struct program_solid *values)
{
	glUseProgram(program.id);
	glUniformMatrix4fv(inputs[MATRIX].loc, 1, GL_FALSE, values->matrix);
}

PROGRAM_REGISTER(&program)
