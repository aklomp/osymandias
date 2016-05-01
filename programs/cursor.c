#include <stdbool.h>
#include <GL/gl.h>

#include "../inlinebin.h"
#include "../programs.h"
#include "cursor.h"

static struct input inputs[] =
	{ [LOC_CURSOR_MAT_PROJ]	= { .name = "mat_proj", .type = TYPE_UNIFORM   }
	, [LOC_CURSOR_VERTEX]	= { .name = "vertex",   .type = TYPE_ATTRIBUTE }
	, [LOC_CURSOR_TEXTURE]	= { .name = "texture",  .type = TYPE_ATTRIBUTE }
	,			  { .name = NULL }
	} ;

static struct program program =
	{ .name     = "cursor"
	, .vertex   = { .src = SHADER_CURSOR_VERTEX }
	, .fragment = { .src = SHADER_CURSOR_FRAGMENT }
	, .inputs   = inputs
	} ;

struct program *
program_cursor (void)
{
	return &program;
}

GLint
program_cursor_loc (const enum LocCursor index)
{
	return inputs[index].loc;
}

void
program_cursor_use (struct program_cursor *values)
{
	glUseProgram(program.id);
	glUniformMatrix4fv(inputs[LOC_CURSOR_MAT_PROJ].loc, 1, GL_FALSE, values->mat_proj);
}
