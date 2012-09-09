#ifndef TEXTURE_MGR_H
#define TEXTURE_MGR_H

struct texture {
	GLuint id;
	int zoomfactor;
	unsigned int offset_x;
	unsigned int offset_y;
};

int texture_mgr_init (void);
void texture_mgr_destroy (void);
struct texture *texture_request (const unsigned int zoom, const unsigned int xn, const unsigned int yn);

#endif	/* TEXTURE_MGR_H */
