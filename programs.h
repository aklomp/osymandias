#ifndef PROGRAMS_H
#define PROGRAMS_H

struct shader {
	enum inlinebin	 src;
	const uint8_t	*buf;
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
	struct shader	 fragment;
	struct shader	 vertex;
	GLuint		 id;
	struct input	*inputs;
};

bool programs_init (void);

#endif	/* PROGRAMS_H */
