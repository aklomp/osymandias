#include <stdbool.h>
#include <GL/gl.h>

#include "../inlinebin.h"
#include "../programs.h"
#include "bkgd.h"

enum	{ VERTEX
	, TEXTURE
	, TEX
	} ;

static struct input inputs[] =
	{ [VERTEX]	= { .name = "vertex",  .type = TYPE_ATTRIBUTE }
	, [TEXTURE]	= { .name = "texture", .type = TYPE_ATTRIBUTE }
	, [TEX]		= { .name = "tex",     .type = TYPE_UNIFORM }
	,		  { .name = NULL }
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
	return inputs[VERTEX].loc;
}

GLint
program_bkgd_loc_texture (void)
{
	return inputs[TEXTURE].loc;
}

void
program_bkgd_use (struct program_bkgd *values)
{
	glUseProgram(program.id);
	glUniform1i(inputs[TEX].loc, values->tex);
}
