#ifndef TILEDRAWER_H
#define TILEDRAWER_H

struct tiledrawer {
	float x;
	float y;
	float wd;
	float ht;
	GLuint texture_id;
	struct quadtree_req *req;
	float *pos;
};

void tiledrawer (const struct tiledrawer *);

#endif	/* TILEDRAWER_H */
