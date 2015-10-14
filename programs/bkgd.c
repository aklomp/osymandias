#include <stdbool.h>
#include <GL/gl.h>

#include "../inlinebin.h"
#include "../programs.h"
#include "bkgd.h"

static struct input inputs[] =
	{ { .name = "vertex",  .type = TYPE_ATTRIBUTE }
	, { .name = "texture", .type = TYPE_ATTRIBUTE }
	, { .name = "tex",     .type = TYPE_UNIFORM }
	, {  NULL }
	} ;

static struct program program =
	{ .name     = "bkgd"
	, .vertex   = { .src = SHADER_BKGD_VERTEX }
	, .fragment = { .src = SHADER_BKGD_FRAGMENT }
	, .inputs   = inputs
	} ;

struct program *
program_bkgd (void)
{
	return &program;
}

GLint
program_bkgd_loc_vertex (void)
{
	return inputs[0].loc;
}

GLint
program_bkgd_loc_texture (void)
{
	return inputs[1].loc;
}

void
program_bkgd_use (struct program_bkgd *values)
{
	glUseProgram(program.id);
	glUniform1i(inputs[2].loc, values->tex);
}
