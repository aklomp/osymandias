#include "layer.h"
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
		if (layer->on_paint)
			layer->on_paint(cam, vp);
}

void
layers_resize (const struct viewport *vp)
{
	FOREACH_LAYER
		if (layer->on_resize)
			layer->on_resize(vp);
}

void
layers_destroy (void)
{
	FOREACH_LAYER
		if (layer->on_destroy)
			layer->on_destroy();
}

bool
layers_init (const struct viewport *vp)
{
	FOREACH_LAYER
		if (layer->on_init && layer->on_init(vp) == false) {
			layers_destroy();
			return false;
		}

	return true;
}
