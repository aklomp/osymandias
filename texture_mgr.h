#ifndef TEXTURE_MGR_H
#define TEXTURE_MGR_H

struct xylist_req;

struct texture {
	GLuint id;
	int zoomfactor;
	unsigned int offset_x;
	unsigned int offset_y;
};

int texture_mgr_init (void);
void texture_mgr_destroy (void);
struct texture *texture_request (struct xylist_req *req);
int texture_area_is_covered (const unsigned int zoom, const unsigned int xmin, const unsigned int ymin, const unsigned int xmax, const unsigned int ymax);
void texture_zoom_change (const unsigned int zoom);

#endif	/* TEXTURE_MGR_H */
