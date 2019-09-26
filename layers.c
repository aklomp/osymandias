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
layers_paint (const struct camera *cam, const struct viewport *vp)
{
	FOREACH_LAYER
		if (layer->paint)
			layer->paint(cam, vp);
}

void
layers_resize (const struct viewport *vp)
{
	FOREACH_LAYER
		if (layer->resize)
			layer->resize(vp);
}

void
layers_destroy (void)
{
	FOREACH_LAYER
		if (layer->destroy)
			layer->destroy();
}

bool
layers_init (const struct viewport *vp)
{
	FOREACH_LAYER
		if (layer->init && layer->init(vp) == false) {
			layers_destroy();
			return false;
		}

	return true;
}
