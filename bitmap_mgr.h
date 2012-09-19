#ifndef BITMAP_MGR_H
#define BITMAP_MGR_H

struct xylist_req;

bool bitmap_mgr_init (void);
void bitmap_mgr_destroy (void);
void *bitmap_request (struct xylist_req *req);
void bitmap_zoom_change (const unsigned int zoom);

#endif	/* BITMAP_MGR_H */
