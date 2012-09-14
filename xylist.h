#ifndef XYLIST_H
#define XYLIST_H

struct xylist;

struct xylist_req {
	unsigned int xn;		// x index of requested tile
	unsigned int yn;		// y index of requested tile
	unsigned int zoom;		// zoom level of requested tile
	unsigned int xmin;		// leftmost column on viewport
	unsigned int ymin;		// topmost row on viewport
	unsigned int xmax;		// rightmost column on viewport
	unsigned int ymax;		// bottommost row on viewport
	unsigned int search_depth;	// how far down to propagate the request
};

struct xylist *
xylist_create (const unsigned int zoom_min, const unsigned int zoom_max, const unsigned int tiles_max,
		void *(*callback_procure)(struct xylist_req *req),
		void  (*callback_destroy)(void *data));

void xylist_destroy (struct xylist **l);
void *xylist_request (struct xylist *l, struct xylist_req *req);
bool xylist_insert_tile (struct xylist *l, const unsigned int zoom, const unsigned int xn, const unsigned int yn, void *const data);
bool xylist_area_is_covered (struct xylist *l, const struct xylist_req *const req);

#endif /* XYLIST_H */
