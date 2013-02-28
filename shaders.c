#include <stdbool.h>
#include <stdio.h>

#include <GL/gl.h>

enum shaders {
	SHADER_CURSOR,
	NUM_SHADERS
};

static const char *fs_cursor =
#include "cursor.glsl.h"

static GLuint progs[NUM_SHADERS] = { 0 };
static const char **fs[NUM_SHADERS] = { &fs_cursor };

static bool
shader_compile (const enum shaders n)
{
	GLint param;
	GLuint s = glCreateShader(GL_FRAGMENT_SHADER);

	glShaderSource(s, 1, fs[n], 0);
	glCompileShader(s);
	glGetShaderiv(s, GL_COMPILE_STATUS, &param);
	if (param == GL_FALSE) {
		char buf[512];
		GLsizei len;
		glGetShaderInfoLog(s, sizeof(buf), &len, buf);
		buf[len] = '\0';
		printf("%s\n", buf);
		return false;
	}
	progs[n] = glCreateProgram();
	glAttachShader(progs[n], s);
	glLinkProgram(progs[n]);
	return true;
}

void
shaders_init (void)
{
	static bool init = false;

	// Ensure we run only once:
	if (init) return; else init = true;

	for (enum shaders n = 0; n < NUM_SHADERS; n++) {
		shader_compile(n);
	}
}

static void
rebind_float (float *cur, const float new, GLuint *loc, const char *name, const enum shaders n)
{
	// Only rebind parameter if changed:
	if (*cur == new) return;

	// Only get location if we don't already have it:
	if (*loc == 0) *loc = glGetUniformLocation(progs[n], name);

	glUniform1f(*loc, new);
	*cur = new;
}

#if 0
static void
rebind_int (int *cur, const int new, GLuint *loc, const char *name, const enum shaders n)
{
	// Only rebind parameter if changed:
	if (*cur == new) return;

	// Only get location if we don't already have it:
	if (*loc == 0) *loc = glGetUniformLocation(progs[n], name);

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

	glUseProgram(progs[SHADER_CURSOR]);
	rebind_float(&halfwd, cur_halfwd, &halfwd_loc, "halfwd", SHADER_CURSOR);
	rebind_float(&halfht, cur_halfht, &halfht_loc, "halfht", SHADER_CURSOR);
}
