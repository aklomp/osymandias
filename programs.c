#include <stdlib.h>
#include <stdio.h>

#include "programs.h"

// Start and end of linker array:
extern const void programs_list_start;
extern const void programs_list_end;

// Pointers to start and end of linker array:
static struct program *list_start = (void *) &programs_list_start;
static struct program *list_end   = (void *) &programs_list_end;

// Local helper struct for compiling shaders:
struct shadermeta {
	struct shader	*shader;
	const char	*progname;
	const char	*typename;
	GLenum		 type;
};

static bool
compile_success (const struct shadermeta *meta)
{
	GLint	status;
	GLsizei	length;

	// Get compile status:
	glGetShaderiv(meta->shader->id, GL_COMPILE_STATUS, &status);

	// Get size of compile log:
	glGetShaderiv(meta->shader->id, GL_INFO_LOG_LENGTH, &length);

	// Print compile log:
	if (length > 1) {
		GLchar *log = calloc(length, sizeof(GLchar));
		glGetShaderInfoLog(meta->shader->id, length, NULL, log);
		fprintf(stderr, "%s: %s: glCompileShader %s:\n%s\n",
			meta->progname, meta->typename,
			(status == GL_TRUE) ? "warning" : "error", log);
		free(log);
	}

	return (status == GL_TRUE);
}

static bool
link_success (struct program *program)
{
	GLint status;
	GLsizei length;

	glGetProgramiv(program->id, GL_LINK_STATUS, &status);
	if (status != GL_FALSE)
		return true;

	glGetProgramiv(program->id, GL_INFO_LOG_LENGTH, &length);
	GLchar *log = calloc(length, sizeof(GLchar));
	glGetProgramInfoLog(program->id, length, NULL, log);
	fprintf(stderr, "%s: glLinkProgram failed: %s\n", program->name, log);
	free(log);
	return false;
}

static bool
shader_compile (struct shadermeta *meta)
{
	struct shader *shader = meta->shader;

	inlinebin_get(shader->src, &shader->buf, &shader->len);

	const GLchar *const *buf = (const GLchar *const *) &shader->buf;
	const GLint         *len = (const GLint *)         &shader->len;

	shader->id = glCreateShader(meta->type);
	glShaderSource(shader->id, 1, buf, len);
	glCompileShader(shader->id);

	if (compile_success(meta))
		return true;

	glDeleteShader(shader->id);
	return false;
}

static bool
shader_fragment_create (struct program *program)
{
	if (program->fragment.src == INLINEBIN_NONE)
		return true;

	struct shadermeta meta = {
		.shader		= &program->fragment,
		.progname	= program->name,
		.typename	= "fragment",
		.type		= GL_FRAGMENT_SHADER
	};

	if (!shader_compile(&meta))
		return false;

	glAttachShader(program->id, meta.shader->id);
	return true;
}

static bool
shader_vertex_create (struct program *program)
{
	if (program->vertex.src == INLINEBIN_NONE)
		return true;

	struct shadermeta meta = {
		.shader		= &program->vertex,
		.progname	= program->name,
		.typename	= "vertex",
		.type		= GL_VERTEX_SHADER
	};

	if (!shader_compile(&meta))
		return false;

	glAttachShader(program->id, meta.shader->id);
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
program_link (struct program *program)
{
	bool ret = false;

	if (!shader_vertex_create(program))
		return false;

	if (!shader_fragment_create(program))
		goto err;

	glLinkProgram(program->id);
	ret = true;

	shader_fragment_destroy(program);
err:	shader_vertex_destroy(program);
	return ret;
}

static bool
input_link (struct program *program, struct input *i)
{
	const char *type = "";

	switch (i->type)
	{
	case TYPE_UNIFORM:
		type = "uniform";
		i->loc = glGetUniformLocation(program->id, i->name);
		break;

	case TYPE_ATTRIBUTE:
		type = "attribute";
		i->loc = glGetAttribLocation(program->id, i->name);
		break;
	}

	if (i->loc >= 0)
		return true;

	fprintf(stderr, "%s: could not link %s '%s'\n", program->name, type, i->name);
	return false;
}

static bool
program_create (struct program *program)
{
	program->id = glCreateProgram();
	program->created = true;

	if (!program_link(program))
		return false;

	if (!link_success(program))
		return false;

	for (struct input *input = program->inputs; input->name; input++)
		if (!input_link(program, input))
			return false;

	return true;
}

void
programs_destroy (void)
{
	for (struct program *p = list_start; p != list_end; p++)
		if (p->created)
			glDeleteProgram(p->id);
}

bool
programs_init (void)
{
	for (struct program *p = list_start; p != list_end; p++)
		if (!program_create(p)) {
			programs_destroy();
			return false;
		}

	return true;
}

void
program_none (void)
{
	glUseProgram(0);
}
