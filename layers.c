#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include "layers.h"
#include "layers/background.h"
#include "layers/basemap.h"
#include "layers/copyright.h"
#include "layers/cursor.h"
#include "layers/osm.h"
#include "layers/overview.h"

#define FOREACH_LAYER \
	for (struct layer **l = layers, *layer; \
		l < layers + nlayers && (layer = *l); \
		l++)

// Master list of layers, filled by layers_init():
static struct layer **layers = NULL;

// Number of layers:
static int nlayers = 0;

void
layers_paint (void)
{
	for (int i = 0; i < nlayers; i++)
		if (layers[i]->paint)
			layers[i]->paint();
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

	free(layers);
}

bool
layers_init (void)
{
	const struct layer *l[] = {
		layer_background(),
		layer_basemap(),
		layer_osm(),
		layer_copyright(),
		layer_overview(),
		layer_cursor(),
	};

	// Count the number of layers:
	nlayers = sizeof(l) / sizeof(l[0]);

	// Allocate space for static list:
	if ((layers = malloc(sizeof(l))) == NULL)
		return false;

	// Copy to static list:
	memcpy(layers, l, sizeof(l));

	FOREACH_LAYER
		if (layer->init && layer->init() == false) {
			layers_destroy();
			return false;
		}

	return true;
}
