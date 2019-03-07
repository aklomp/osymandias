#include <stdbool.h>
#include <GL/gl.h>

#include "../inlinebin.h"
#include "../programs.h"
#include "tile2d.h"

static struct input inputs[] =
	{ [LOC_TILE2D_MAT_PROJ]	= { .name = "mat_proj", .type = TYPE_UNIFORM   }
	, [LOC_TILE2D_VERTEX]	= { .name = "vertex",   .type = TYPE_ATTRIBUTE }
	, [LOC_TILE2D_TEXTURE]	= { .name = "texture",  .type = TYPE_ATTRIBUTE }
	,			  { .name = NULL }
	} ;

struct program program_tile2d __attribute__((section(".programs"))) =
	{ .name     = "tile2d"
	, .vertex   = { .src = SHADER_TILE2D_VERTEX }
	, .fragment = { .src = SHADER_TILE2D_FRAGMENT }
	, .inputs   = inputs
	} ;

GLint
program_tile2d_loc (const enum LocCursor index)
{
	return inputs[index].loc;
}

void
program_tile2d_use (struct program_tile2d *values)
{
	glUseProgram(program_tile2d.id);
	glUniformMatrix4fv(inputs[LOC_TILE2D_MAT_PROJ].loc, 1, GL_FALSE, values->mat_proj);
}
