#ifndef BITMAP_MGR_H
#define BITMAP_MGR_H

bool bitmap_mgr_init (void);
void bitmap_mgr_destroy (void);
void *bitmap_request (const unsigned int zoom, const unsigned int x, const unsigned int y);

#endif	/* BITMAP_MGR_H */
