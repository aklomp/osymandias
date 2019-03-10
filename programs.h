#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <GL/gl.h>

#include "inlinebin.h"

struct shader {
	enum Inlinebin	 src;
	const char	*buf;
	size_t		 len;
	GLuint		 id;
};

enum input_type {
	TYPE_UNIFORM,
	TYPE_ATTRIBUTE
};

struct input {
	const char	*name;
	enum input_type	 type;
	GLint		 loc;
};

struct program {
	const char	*name;
	struct shader	 fragment;
	struct shader	 vertex;
	GLuint		 id;
	struct input	*inputs;
	bool		 created;
};

extern bool programs_init    (void);
extern void programs_destroy (void);
extern void program_none     (void);
