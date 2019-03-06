#pragma once

#include <stdbool.h>

#define LAYER(Z)	const struct layer layer_ ## Z __attribute__((section(".layer." # Z)))

struct layer {
	bool (*init)    (void);
	void (*paint)   (void);
	void (*zoom)    (const unsigned int zoom);
	void (*resize)  (const unsigned int width, const unsigned int height);
	void (*destroy) (void);
};

extern bool layers_init    (void);
extern void layers_destroy (void);
extern void layers_paint   (void);
extern void layers_zoom    (const unsigned int zoom);
extern void layers_resize  (const unsigned int width, const unsigned int height);
