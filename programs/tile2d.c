#include <stdbool.h>
#include <GL/gl.h>

#include "../inlinebin.h"
#include "../program.h"
#include "tile2d.h"

static struct input inputs[] =
	{ [LOC_TILE2D_MAT_PROJ]	= { .name = "mat_proj", .type = TYPE_UNIFORM   }
	, [LOC_TILE2D_VERTEX]	= { .name = "vertex",   .type = TYPE_ATTRIBUTE }
	, [LOC_TILE2D_TEXTURE]	= { .name = "texture",  .type = TYPE_ATTRIBUTE }
	,			  { .name = NULL }
	} ;

static struct program program = {
	.name     = "tile2d",
	.vertex   = { SHADER_TILE2D_VERTEX },
	.fragment = { SHADER_TILE2D_FRAGMENT },
	.inputs   = inputs,
};

GLint
program_tile2d_loc (const enum LocCursor index)
{
	return inputs[index].loc;
}

void
program_tile2d_use (struct program_tile2d *values)
{
	glUseProgram(program.id);
	glUniformMatrix4fv(inputs[LOC_TILE2D_MAT_PROJ].loc, 1, GL_FALSE, values->mat_proj);
}

PROGRAM_REGISTER(&program)
