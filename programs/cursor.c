#include <stdbool.h>
#include <GL/gl.h>

#include "../inlinebin.h"
#include "../programs.h"
#include "cursor.h"

static struct input inputs[] =
	{ { .name = "halfwd", .type = TYPE_UNIFORM }
	, { .name = "halfht", .type = TYPE_UNIFORM }
	, {  NULL }
	} ;

static struct program program =
	{ .fragment = { .src = SHADER_CURSOR_FRAGMENT }
	, .vertex   = { .src = INLINEBIN_NONE }
	, .inputs   = inputs
	} ;

struct program *
program_cursor (void)
{
	return &program;
}

void
program_cursor_use (struct program_cursor *values)
{
	glUseProgram(program.id);
	glUniform1f(inputs[0].loc, values->halfwd);
	glUniform1f(inputs[1].loc, values->halfht);
}
