#include <stdbool.h>
#include <GL/gl.h>

#include "../inlinebin.h"
#include "../programs.h"
#include "cursor.h"

static struct input inputs[] =
	{ { .name = "mat_proj", .type = TYPE_UNIFORM }
	, { .name = "mat_view", .type = TYPE_UNIFORM }
	, { .name = "vertex",   .type = TYPE_ATTRIBUTE }
	, { .name = NULL }
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
program_cursor_loc_vertex (void)
{
	return inputs[2].loc;
}

void
program_cursor_use (struct program_cursor *values)
{
	glUseProgram(program.id);
	glUniformMatrix4fv(inputs[0].loc, 1, GL_FALSE, values->mat_proj);
	glUniformMatrix4fv(inputs[1].loc, 1, GL_FALSE, values->mat_view);
}
