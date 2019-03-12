#pragma once

#include <stdbool.h>

#include <vec/vec.h>

// This structure contains a tile picked for display by the tilepicker:
struct tilepicker {
	struct {
		float x;
		float y;
	} pos;
	struct {
		float wd;
		float ht;
	} size;
	unsigned int zoom;
};

extern void tilepicker_recalc  (void);
extern bool tilepicker_first   (struct tilepicker *tile);
extern bool tilepicker_next    (struct tilepicker *tile);
extern void tilepicker_destroy (void);
