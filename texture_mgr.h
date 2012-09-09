#ifndef TEXTURE_MGR_H
#define TEXTURE_MGR_H

int texture_mgr_init (void);
void texture_mgr_destroy (void);
GLuint texture_request (const unsigned int zoom, const unsigned int xn, const unsigned int yn);

#endif	/* TEXTURE_MGR_H */
