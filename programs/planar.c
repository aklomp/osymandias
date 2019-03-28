#include "../inlinebin.h"
#include "../programs.h"
#include "planar.h"

static struct input inputs[] =
	{ [LOC_PLANAR_MAT_VIEWPROJ] = { .name = "mat_viewproj", .type = TYPE_UNIFORM   }
	, [LOC_PLANAR_MAT_MODEL]    = { .name = "mat_model",    .type = TYPE_UNIFORM   }
	, [LOC_PLANAR_TILE_ZOOM]    = { .name = "tile_zoom",    .type = TYPE_UNIFORM   }
	, [LOC_PLANAR_WORLD_ZOOM]   = { .name = "world_zoom",   .type = TYPE_UNIFORM   }
	, [LOC_PLANAR_VERTEX]       = { .name = "vertex",       .type = TYPE_ATTRIBUTE }
	, [LOC_PLANAR_TEXTURE]      = { .name = "texture",      .type = TYPE_ATTRIBUTE }
	,                             { .name = NULL }
	} ;

struct program program_planar __attribute__((section(".programs"))) =
	{ .name     = "planar"
	, .vertex   = { .src = SHADER_PLANAR_VERTEX   }
	, .fragment = { .src = SHADER_PLANAR_FRAGMENT }
	, .inputs   = inputs
	} ;

void
program_planar_set_tile_zoom (const int zoom)
{
	glUniform1i(inputs[LOC_PLANAR_TILE_ZOOM].loc, zoom);
}

GLint
program_planar_loc (const enum LocPlanar index)
{
	return inputs[index].loc;
}

void
program_planar_use (struct program_planar *values)
{
	glUseProgram(program_planar.id);
	glUniformMatrix4fv(inputs[LOC_PLANAR_MAT_VIEWPROJ].loc, 1, GL_FALSE, values->mat_viewproj);
	glUniformMatrix4fv(inputs[LOC_PLANAR_MAT_MODEL].loc, 1, GL_FALSE, values->mat_model);
	glUniform1i(inputs[LOC_PLANAR_TILE_ZOOM].loc, values->tile_zoom);
	glUniform1i(inputs[LOC_PLANAR_WORLD_ZOOM].loc, values->world_zoom);
}
