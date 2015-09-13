#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <GL/gl.h>

#include "inlinebin.h"

struct shader {
	enum inlinebin	 src;
	const uint8_t	*buf;
	size_t		 len;
	GLuint		 id;
};

enum input_type
	{ TYPE_UNIFORM
	, TYPE_ATTRIBUTE
	} ;

struct input {
	const char	*name;
	enum input_type	 type;
	GLint		 loc;
};

struct program {
	struct shader	fragment;
	struct shader	vertex;
	GLuint		id;
	struct input	inputs[];
};

static struct program shader_cursor =
	{ .fragment = { .src = SHADER_CURSOR_FRAGMENT }
	, .vertex   = { .src = INLINEBIN_NONE }
	, .inputs   =
	  { { .name = "halfwd", .type = TYPE_UNIFORM }
	  , { .name = "halfht", .type = TYPE_UNIFORM }
	  , {  NULL }
	  }
	} ;

// All programs:
static struct program *programs[] =
	{ &shader_cursor
	} ;

static bool
compile_success (GLuint shader)
{
	GLint status, length;

	glGetShaderiv(shader, GL_COMPILE_STATUS, &status);
	if (status != GL_FALSE)
		return true;

	glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &length);
	GLchar *log = calloc(length, sizeof(GLchar));
	glGetShaderInfoLog(shader, length, NULL, log);
	fprintf(stderr, "glCompileShader failed:\n%s\n", log);
	free(log);
	return false;
}

static bool
link_success (GLuint program)
{
	GLint status, length;

	glGetProgramiv(program, GL_LINK_STATUS, &status);
	if (status != GL_FALSE)
		return true;

	glGetProgramiv(program, GL_INFO_LOG_LENGTH, &length);
	GLchar *log = calloc(length, sizeof(GLchar));
	glGetProgramInfoLog(program, length, NULL, log);
	fprintf(stderr, "glLinkProgram failed: %s\n", log);
	free(log);
	return false;
}

static bool
shader_compile (struct shader *shader, GLenum type)
{
	inlinebin_get(shader->src, &shader->buf, &shader->len);

	shader->id = glCreateShader(type);
	glShaderSource(shader->id, 1, (const GLchar * const *)&shader->buf, (const GLint *)&shader->len);
	glCompileShader(shader->id);

	if (compile_success(shader->id))
		return true;

	glDeleteShader(shader->id);
	return false;
}

static bool
shader_fragment_create (struct program *program)
{
	if (program->fragment.src == INLINEBIN_NONE)
		return true;

	if (!shader_compile(&program->fragment, GL_FRAGMENT_SHADER))
		return false;

	glAttachShader(program->id, program->fragment.id);
	return true;
}

static bool
shader_vertex_create (struct program *program)
{
	if (program->vertex.src == INLINEBIN_NONE)
		return true;

	if (!shader_compile(&program->vertex, GL_VERTEX_SHADER))
		return false;

	glAttachShader(program->id, program->vertex.id);
	return true;
}

static void
shader_fragment_destroy (struct program *program)
{
	if (program->fragment.src == INLINEBIN_NONE)
		return;

	glDetachShader(program->id, program->fragment.id);
	glDeleteShader(program->fragment.id);
}

static void
shader_vertex_destroy (struct program *program)
{
	if (program->vertex.src == INLINEBIN_NONE)
		return;

	glDetachShader(program->id, program->vertex.id);
	glDeleteShader(program->vertex.id);
}

static bool
input_link (GLuint program_id, struct input *i)
{
	switch (i->type)
	{
	case TYPE_UNIFORM:
		i->loc = glGetUniformLocation(program_id, i->name);
		break;

	case TYPE_ATTRIBUTE:
		i->loc = glGetAttribLocation(program_id, i->name);
		break;
	}

	if (i->loc >= 0)
		return true;

	fprintf(stderr, "Could not link %s\n", i->name);
	return false;
}

static bool
program_create (struct program *program)
{
	program->id = glCreateProgram();

	if (!(shader_vertex_create(program)
	   && shader_fragment_create(program)))
		goto err;

	glLinkProgram(program->id);

	shader_vertex_destroy(program);
	shader_fragment_destroy(program);

	if (!link_success(program->id))
		goto err;

	for (struct input *input = program->inputs; input->name; input++)
		if (!input_link(program->id, input))
			goto err;

	return true;

err:	glDeleteProgram(program->id);
	return false;
}

bool
shaders_init (void)
{
	for (size_t i = 0; i < sizeof(programs) / sizeof(programs[0]); i++)
		if (!program_create(programs[i]))
			return false;

	return true;
}

void
shader_use_cursor (const float halfwd, const float halfht)
{
	glUseProgram(programs[0]->id);
	glUniform1f(programs[0]->inputs[0].loc, halfwd);
	glUniform1f(programs[0]->inputs[1].loc, halfht);
}
