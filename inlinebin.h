#ifndef INLINEBIN_H
#define INLINEBIN_H

enum inlinebin {
	INLINEBIN_NONE,
	SHADER_CURSOR_VERTEX,
	SHADER_CURSOR_FRAGMENT,
	SHADER_BKGD_VERTEX,
	SHADER_BKGD_FRAGMENT,
	SHADER_SOLID_VERTEX,
	SHADER_SOLID_FRAGMENT,
};

void inlinebin_get (enum inlinebin member, const void **buf, size_t *len);

#endif	/* INLINEBIN_H */
