#pragma once

struct quadtree_req;

bool bitmap_mgr_init (void);
void bitmap_mgr_destroy (void);
int bitmap_request (struct quadtree_req *req);
void bitmap_zoom_change (const int zoomlevel);
