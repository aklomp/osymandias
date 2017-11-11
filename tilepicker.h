#include <vec/vec.h>

// This structure contains a tile picked for display by the tilepicker:
struct tilepicker {
	union vec coords[4];
	union vec normal[4];
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

void tilepicker_recalc (void);
bool tilepicker_first (struct tilepicker *tile);
bool tilepicker_next (struct tilepicker *tile);
void tilepicker_destroy (void);
