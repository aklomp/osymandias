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

struct program {
	struct shader	*fragment;
	struct shader	*vertex;
	GLuint		 id;
};

// Cursor, fragment shader:
static struct shader shader_cursor_fragment =
	{ SHADER_CURSOR_FRAGMENT };

// All programs:
static struct program programs[] =
{ { .fragment = &shader_cursor_fragment
  , .vertex   = NULL
  }
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
	if (!program->fragment)
		return true;

	if (!shader_compile(program->fragment, GL_FRAGMENT_SHADER))
		return false;

	glAttachShader(program->id, program->fragment->id);
	return true;
}

static bool
shader_vertex_create (struct program *program)
{
	if (!program->vertex)
		return true;

	if (!shader_compile(program->vertex, GL_VERTEX_SHADER))
		return false;

	glAttachShader(program->id, program->vertex->id);
	return true;
}

static void
shader_fragment_destroy (struct program *program)
{
	if (!program->fragment)
		return;

	glDetachShader(program->id, program->fragment->id);
	glDeleteShader(program->fragment->id);
}

static void
shader_vertex_destroy (struct program *program)
{
	if (!program->vertex)
		return;

	glDetachShader(program->id, program->vertex->id);
	glDeleteShader(program->vertex->id);
}

static bool
program_create (struct program *program)
{
	program->id = glCreateProgram();

	if (!(shader_vertex_create(program)
	   && shader_fragment_create(program))) {
		glDeleteProgram(program->id);
		return false;
	}

	glLinkProgram(program->id);

	shader_vertex_destroy(program);
	shader_fragment_destroy(program);

	if (link_success(program->id))
		return true;

	glDeleteProgram(program->id);
	return false;
}

bool
shaders_init (void)
{
	for (size_t i = 0; i < sizeof(programs) / sizeof(programs[0]); i++)
		if (!program_create(&programs[i]))
			return false;

	return true;
}

static void
rebind_float (float *cur, const float new, GLuint *loc, const char *name)
{
	// Only rebind parameter if changed:
	if (*cur == new)
		return;

	// Only get location if we don't already have it:
	if (*loc == 0)
		*loc = glGetUniformLocation(programs[0].id, name);

	glUniform1f(*loc, new);
	*cur = new;
}

#if 0
static void
rebind_int (int *cur, const int new, GLuint *loc, const char *name)
{
	// Only rebind parameter if changed:
	if (*cur == new)
		return;

	// Only get location if we don't already have it:
	if (*loc == 0)
		*loc = glGetUniformLocation(programs[0].id, name);

	glUniform1i(*loc, new);
	*cur = new;
}
#endif

void
shader_use_cursor (const float cur_halfwd, const float cur_halfht)
{
	static float halfwd = 0.0;
	static float halfht = 0.0;
	static GLuint halfwd_loc = 0;
	static GLuint halfht_loc = 0;

	glUseProgram(programs[0].id);
	rebind_float(&halfwd, cur_halfwd, &halfwd_loc, "halfwd");
	rebind_float(&halfht, cur_halfht, &halfht_loc, "halfht");
}
