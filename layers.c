#include "layer.h"
#include "layers.h"

// Pointer to the first layer in the linked list.
static struct layer *layer_list;

#define FOREACH_LAYER \
	for (struct layer *layer = layer_list; layer; layer = layer->next)

void
layers_paint (const struct camera *cam, const struct viewport *vp)
{
	FOREACH_LAYER
		if (layer->on_paint && layer->visible)
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
		if (layer->on_destroy && layer->created)
			layer->on_destroy();
}

bool
layers_init (const struct viewport *vp)
{
	FOREACH_LAYER {
		if (layer->on_init && layer->on_init(vp)) {
			layer->created = true;
			continue;
		}

		layers_destroy();
		return false;
	}

	return true;
}

void
layers_link (struct layer *layer)
{
	struct layer *l;

	// If this is the first layer to register itself, or if it sorts below
	// the current list head, then insert it at the front.
	if (layer_list == NULL || layer->zdepth < layer_list->zdepth) {
		layer->next = layer_list;
		layer_list  = layer;
		return;
	}

	// Walk the linked list to find either the last element, or the element
	// whose next element sorts above this layer. This yields the element
	// after which the layer should be inserted.
	for (l = layer_list; l->next != NULL; l = l->next)
		if (l->next->zdepth > layer->zdepth)
			break;

	// Insert the given layer at this location.
	layer->next = l->next;
	l->next     = layer;
}
