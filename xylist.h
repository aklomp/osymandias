#ifndef XYLIST_H
#define XYLIST_H

struct xylist;

struct xylist *
xylist_create (const unsigned int zoom_min, const unsigned int zoom_max, const unsigned int tiles_max,
		void *(*callback_procure)(unsigned int zoom, unsigned int xn, unsigned int yn, const unsigned int search_depth),
		void  (*callback_destroy)(void *data));

void xylist_destroy (struct xylist **l);
void *xylist_request (struct xylist *l, const unsigned int zoom, const unsigned int xn, const unsigned int yn, const unsigned int search_depth);
bool xylist_area_is_covered (struct xylist *l, const unsigned int zoom, const unsigned int tile_xmin, const unsigned int tile_ymin, const unsigned int tile_xmax, const unsigned int tile_ymax);

#endif /* XYLIST_H */
