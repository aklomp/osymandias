#include <stdbool.h>
#include <stdio.h>

#include <GL/gl.h>

enum shaders {
	SHADER_BKGD,
	SHADER_CURSOR,
	SHADER_TILE,
	NUM_SHADERS
};

static const char *fs_bkgd =
#include "bkgd.glsl.h"

static const char *fs_cursor =
#include "cursor.glsl.h"

static const char *fs_tile =
#include "tile.glsl.h"

static GLuint progs[NUM_SHADERS] = { 0, 0 };
static const char **fs[NUM_SHADERS] = { &fs_bkgd, &fs_cursor, &fs_tile };

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

void
shader_use_bkgd (const float cur_maxsize, const int cur_offs_x, const int cur_offs_y)
{
	static int offs_x;
	static int offs_y;
	static float maxsize = 0.0;
	static GLuint offs_x_loc = 0;
	static GLuint offs_y_loc = 0;
	static GLuint maxsize_loc = 0;

	glUseProgram(progs[SHADER_BKGD]);
	rebind_float(&maxsize, cur_maxsize, &maxsize_loc, "maxsize", SHADER_BKGD);
	rebind_int(&offs_x, cur_offs_x, &offs_x_loc, "offs_x", SHADER_BKGD);
	rebind_int(&offs_y, cur_offs_y, &offs_y_loc, "offs_y", SHADER_BKGD);
}

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

void
shader_use_tile (const int cur_offs_x, const int cur_offs_y, const int texture_offs_x, const int texture_offs_y, const int zoomfactor)
{
	static int offs_x = 0;
	static int offs_y = 0;
	static int s_texture_offs_x = 0;
	static int s_texture_offs_y = 0;
	static float s_zoomfactor = 0.0;
	static GLuint offs_x_loc = 0;
	static GLuint offs_y_loc = 0;
	static GLuint texture_offs_x_loc = 0;
	static GLuint texture_offs_y_loc = 0;
	static GLuint zoomfactor_loc = 0;

	glUseProgram(progs[SHADER_TILE]);
	rebind_int(&offs_x, cur_offs_x, &offs_x_loc, "offs_x", SHADER_TILE);
	rebind_int(&offs_y, cur_offs_y, &offs_y_loc, "offs_y", SHADER_TILE);
	rebind_int(&s_texture_offs_x, texture_offs_x, &texture_offs_x_loc, "texture_offs_x", SHADER_TILE);
	rebind_int(&s_texture_offs_y, texture_offs_y, &texture_offs_y_loc, "texture_offs_y", SHADER_TILE);
	/* 1:1 is 256.0, 2:1 is 512.0, etc */
	rebind_float(&s_zoomfactor, (float)(1U << (7 + zoomfactor)), &zoomfactor_loc, "zoomfactor", SHADER_TILE);
}
