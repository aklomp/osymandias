#ifndef TILEDRAWER_H
#define TILEDRAWER_H

struct tiledrawer {
	struct vector *coords;
	struct vector *normal;
	float x;
	float y;
	float wd;
	float ht;
	struct {
		unsigned int world;
		unsigned int found;
	} zoom;
	GLuint texture_id;
};

void tiledrawer (const struct tiledrawer *);

#endif	/* TILEDRAWER_H */
