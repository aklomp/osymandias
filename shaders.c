#include <stdbool.h>
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
shader_compile (struct shader *shader, GLenum type)
{
	// Fetch inline data:
	inlinebin_get(shader->src, &shader->buf, &shader->len);

	// Create shader:
	shader->id = glCreateShader(type);
	glShaderSource(shader->id, 1, (const GLchar * const *)&shader->buf, (const GLint *)&shader->len);
	glCompileShader(shader->id);

	// Check compilation status:
	GLint param;
	glGetShaderiv(shader->id, GL_COMPILE_STATUS, &param);
	if (param == GL_TRUE)
		return true;

	// Print error message:
	char buf[512];
	GLsizei len;
	glGetShaderInfoLog(shader->id, sizeof(buf), &len, buf);
	buf[len] = '\0';
	printf("%s\n", buf);
	return false;
}

static bool
program_create (struct program *program)
{
	// Create program:
	program->id = glCreateProgram();

	// Create fragment shader:
	if (program->fragment) {
		if (!shader_compile(program->fragment, GL_FRAGMENT_SHADER)) {
			return false;
		}
		glAttachShader(program->id, program->fragment->id);
	}

	// Create vertex shader:
	if (program->vertex) {
		if (!shader_compile(program->vertex, GL_VERTEX_SHADER)) {
			return false;
		}
		glAttachShader(program->id, program->vertex->id);
	}

	// Link program:
	glLinkProgram(program->id);

	// Check linking status:
	GLint param;
	glGetProgramiv(program->id, GL_LINK_STATUS, &param);
	if (param == GL_TRUE)
		return true;

	// Print error message:
	char buf[512];
	GLsizei len;
	glGetProgramInfoLog(program->id, sizeof(buf), &len, buf);
	buf[len] = '\0';
	printf("%s\n", buf);
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
