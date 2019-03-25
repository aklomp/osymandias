#include "../inlinebin.h"
#include "../programs.h"
#include "spherical.h"

static struct input inputs[] =
	{ [LOC_SPHERICAL_MAT_VIEWPROJ] = { .name = "mat_viewproj", .type = TYPE_UNIFORM   }
	, [LOC_SPHERICAL_MAT_MODEL]    = { .name = "mat_model",    .type = TYPE_UNIFORM   }
	, [LOC_SPHERICAL_WORLD_ZOOM]   = { .name = "world_zoom",   .type = TYPE_UNIFORM   }
	, [LOC_SPHERICAL_VERTEX]       = { .name = "vertex",       .type = TYPE_ATTRIBUTE }
	, [LOC_SPHERICAL_TEXTURE]      = { .name = "texture",      .type = TYPE_ATTRIBUTE }
	,                                { .name = NULL }
	} ;

struct program program_spherical __attribute__((section(".programs"))) =
	{ .name     = "spherical"
	, .vertex   = { .src = SHADER_SPHERICAL_VERTEX   }
	, .fragment = { .src = SHADER_SPHERICAL_FRAGMENT }
	, .inputs   = inputs
	} ;

GLint
program_spherical_loc (const enum LocSpherical index)
{
	return inputs[index].loc;
}

void
program_spherical_use (struct program_spherical *values)
{
	glUseProgram(program_spherical.id);
	glUniformMatrix4fv(inputs[LOC_SPHERICAL_MAT_VIEWPROJ].loc, 1, GL_FALSE, values->mat_viewproj);
	glUniformMatrix4fv(inputs[LOC_SPHERICAL_MAT_MODEL].loc, 1, GL_FALSE, values->mat_model);
	glUniform1i(inputs[LOC_SPHERICAL_WORLD_ZOOM].loc, values->world_zoom);
}
