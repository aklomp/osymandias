#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <GL/gl.h>

#include "inlinebin.h"
#include "programs.h"
#include "programs/cursor.h"

// Master list of programs, filled by programs_init():
static struct program *programs[1];

static bool
compile_success (GLuint shader)
{
	GLint status;
	GLsizei length;

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
	GLint status;
	GLsizei length;

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

	const GLchar *const *buf = (const GLchar *const *) &shader->buf;
	const GLint         *len = (const GLint *)         &shader->len;

	shader->id = glCreateShader(type);
	glShaderSource(shader->id, 1, buf, len);
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
	program->created = true;

	if (!(shader_vertex_create(program)
	   && shader_fragment_create(program)))
		return false;

	glLinkProgram(program->id);

	shader_vertex_destroy(program);
	shader_fragment_destroy(program);

	if (!link_success(program->id))
		return false;

	for (struct input *input = program->inputs; input->name; input++)
		if (!input_link(program->id, input))
			return false;

	return true;
}

void
programs_destroy (void)
{
	for (size_t i = 0; i < sizeof(programs) / sizeof(programs[0]); i++)
		if (programs[i]->created)
			glDeleteProgram(programs[i]->id);
}

bool
programs_init (void)
{
	struct program *p[] = {
		program_cursor(),
	};

	// Copy to static list:
	memcpy(programs, p, sizeof(programs));

	for (size_t i = 0; i < sizeof(programs) / sizeof(programs[0]); i++)
		if (!program_create(programs[i])) {
			programs_destroy();
			return false;
		}

	return true;
}
