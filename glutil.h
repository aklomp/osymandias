#pragma once

// Glutil vertex with only world coordinates:
struct glutil_vertex {
	float x;
	float y;
} __attribute__((packed));

// Glutil vertex with world and texture coordinates:
struct glutil_vertex_uv {
	float x;
	float y;
	float u;
	float v;
} __attribute__((packed));

// Initialize uv coordinates to the default bitmap coordinate system:
//
//   3--2      0,0--1,0
//   |  |  ->   |    |
//   0--1      0,1--1,1
//
#define GLUTIL_VERTEX_UV_DEFAULT			\
	{						\
		[0] = { .u = 0.0f, .v = 1.0f },		\
		[1] = { .u = 1.0f, .v = 1.0f },		\
		[2] = { .u = 1.0f, .v = 0.0f },		\
		[3] = { .u = 0.0f, .v = 0.0f },		\
	}

// Operations on vertices:
void glutil_vertex_link    (const GLuint loc_xy);
void glutil_vertex_uv_link (const GLuint loc_xy, const GLuint loc_uv);

// Glutil texture:
struct glutil_texture {
	GLuint		id;
	enum inlinebin	src;
	GLenum		type;
	unsigned int	width;
	unsigned int	height;
};

// Operations on textures:
bool glutil_texture_load (struct glutil_texture *);

// Draw one quad from two triangles:
void glutil_draw_quad (void);
