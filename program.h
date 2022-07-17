#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <GL/gl.h>

#include "inlinebin.h"
#include "programs.h"

#define PROGRAM_REGISTER(PROGRAM)		\
	__attribute__((constructor, used))	\
	static void				\
	link_program (void)			\
	{					\
		programs_link(PROGRAM);		\
	}

struct shader {
	enum Inlinebin src;
	const uint8_t *buf;
	size_t         len;
	GLuint         id;
};

enum input_type {
	TYPE_UNIFORM,
	TYPE_ATTRIBUTE,
};

struct input {
	const char     *name;
	enum input_type type;
	GLint           loc;
};

struct program {

	// Pointer to the next program in the linked list.
	struct program *next;

	const char   *name;
	struct shader fragment;
	struct shader vertex;
	GLuint        id;
	struct input *inputs;
	bool          created;
};

extern void program_none (void);
