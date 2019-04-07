#include <stdbool.h>
#include <GL/gl.h>

#include "../inlinebin.h"
#include "../programs.h"
#include "tilepicker.h"

enum	{ MAT_VIEWPROJ_INV
	, MAT_MODEL_INV
	, VERTEX
	} ;

static struct input inputs[] =
	{ [MAT_VIEWPROJ_INV] = { .name = "mat_viewproj_inv", .type = TYPE_UNIFORM   }
	, [MAT_MODEL_INV]    = { .name = "mat_model_inv",    .type = TYPE_UNIFORM   }
	, [VERTEX]           = { .name = "vertex",           .type = TYPE_ATTRIBUTE }
	,                      { .name = NULL }
	} ;

struct program program_tilepicker __attribute__((section(".programs"))) =
	{ .name     = "tilepicker"
	, .vertex   = { .src = SHADER_TILEPICKER_VERTEX   }
	, .fragment = { .src = SHADER_TILEPICKER_FRAGMENT }
	, .inputs   = inputs
	} ;

int
program_tilepicker_loc_vertex (void)
{
	return inputs[VERTEX].loc;
}

void
program_tilepicker_use (const struct program_tilepicker *values)
{
	glUseProgram(program_tilepicker.id);
	glUniformMatrix4fv(inputs[MAT_VIEWPROJ_INV].loc, 1, GL_FALSE, values->mat_viewproj_inv);
	glUniformMatrix4fv(inputs[MAT_MODEL_INV].loc, 1, GL_FALSE, values->mat_model_inv);
}
