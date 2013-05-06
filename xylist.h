#ifndef XYLIST_H
#define XYLIST_H

struct xylist;

struct xylist_req {
	unsigned int xn;		// x index of requested tile
	unsigned int yn;		// y index of requested tile
	unsigned int zoom;		// zoom level of requested tile
	unsigned int world_zoom;	// zoom level of current world
	unsigned int world_xmin;	// leftmost column on viewport
	unsigned int world_ymin;	// topmost row on viewport
	unsigned int world_xmax;	// rightmost column on viewport
	unsigned int world_ymax;	// bottommost row on viewport
	unsigned int search_depth;	// how far down to propagate the request
	float cx;			// center x, of view center
	float cy;			// center y, of view center
};

struct xylist *
xylist_create (const unsigned int zoom_min, const unsigned int zoom_max, const unsigned int tiles_max,
		void *(*callback_procure)(struct xylist_req *req),
		void  (*callback_destroy)(void *data));

void xylist_destroy (struct xylist **l);
void *xylist_request (struct xylist *l, struct xylist_req *req);
bool xylist_insert_tile (struct xylist *l, struct xylist_req *req, void *const data);
bool xylist_delete_tile (struct xylist *l, struct xylist_req *req);
bool xylist_area_is_covered (struct xylist *l, const struct xylist_req *const req);
void xylist_purge_other_zoomlevels (struct xylist *l, const unsigned int zoom);

#endif /* XYLIST_H */
