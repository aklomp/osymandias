#include <stdbool.h>

#include "layers.h"
#include "layers/background.h"
#include "layers/basemap.h"
#include "layers/copyright.h"
#include "layers/cursor.h"
#include "layers/osm.h"
#include "layers/overview.h"

#define FOREACH_LAYER \
	for (const struct layer **l = layers, *layer; \
		l < layers + (sizeof(layers) / sizeof(layers[0])) && (layer = *l); \
		l++)

// Master list of layers, in Z order from bottom to top:
static const struct layer *layers[] = {
	&layer_background,
	&layer_basemap,
	&layer_osm,
	&layer_copyright,
	&layer_overview,
	&layer_cursor,
};

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
