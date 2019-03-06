#include "layers.h"

// Start and end of linker array:
extern const void layers_list_start;
extern const void layers_list_end;

// Pointers to start and end of linker array:
static const struct layer *list_start = (const void *) &layers_list_start;
static const struct layer *list_end   = (const void *) &layers_list_end;

#define FOREACH_LAYER \
	for (const struct layer *layer = list_start; layer < list_end; layer++)

void
layers_paint (void)
{
	FOREACH_LAYER
		if (layer->paint)
			layer->paint();
}

void
layers_zoom (const unsigned int zoom)
{
	FOREACH_LAYER
		if (layer->zoom)
			layer->zoom(zoom);
}

void
layers_resize (const unsigned int width, const unsigned int height)
{
	FOREACH_LAYER
		if (layer->resize)
			layer->resize(width, height);
}

void
layers_destroy (void)
{
	FOREACH_LAYER
		if (layer->destroy)
			layer->destroy();
}

bool
layers_init (void)
{
	FOREACH_LAYER
		if (layer->init && layer->init() == false) {
			layers_destroy();
			return false;
		}

	return true;
}
