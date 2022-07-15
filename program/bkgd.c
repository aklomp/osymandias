#include <stdbool.h>
#include <GL/gl.h>

#include "../inlinebin.h"
#include "../program.h"
#include "bkgd.h"

static struct input inputs[] =
	{ [LOC_BKGD_VERTEX]	= { .name = "vertex",  .type = TYPE_ATTRIBUTE }
	, [LOC_BKGD_TEXTURE]	= { .name = "texture", .type = TYPE_ATTRIBUTE }
	,			  { .name = NULL }
	} ;

static struct program program = {
	.name     = "bkgd",
	.vertex   = { SHADER_BKGD_VERTEX },
	.fragment = { SHADER_BKGD_FRAGMENT },
	.inputs   = inputs,
};

GLint
program_bkgd_loc (const enum LocBkgd index)
{
	return inputs[index].loc;
}

void
program_bkgd_use (void)
{
	glUseProgram(program.id);
}

PROGRAM_REGISTER(&program)
