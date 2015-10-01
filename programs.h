#ifndef PROGRAMS_H
#define PROGRAMS_H

struct shader {
	enum inlinebin	 src;
	const void	*buf;
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

bool programs_init (void);
void programs_destroy (void);
void program_none (void);

#endif	/* PROGRAMS_H */
