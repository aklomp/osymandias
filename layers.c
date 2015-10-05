#include <stdbool.h>
#include <string.h>

#include "layers.h"
#include "layers/background.h"
#include "layers/blanktile.h"
#include "layers/cursor.h"
#include "layers/osm.h"
#include "layers/overview.h"

#define LAYERS	5

// Master list of layers, filled by layers_init():
static struct layer *layers[LAYERS];

void
layers_paint (void)
{
	int i;

	// Loop over all layers front to back,
	// check if they occlude the background;
	// if they do, then don't bother painting lower layers:
	for (i = LAYERS - 1; i >= 0; i--)
		if (layers[i]->occludes)
			if (layers[i]->occludes())
				break;

	// If none of the layers claimed full occlusion,
	// start with the first layer:
	for (i = (i < 0) ? 0 : i; i < LAYERS; i++)
		if (layers[i]->paint)
			layers[i]->paint();
}

void
layers_zoom (const unsigned int zoom)
{
	for (int i = 0; i < LAYERS; i++)
		if (layers[i]->zoom)
			layers[i]->zoom(zoom);
}

void
layers_destroy (void)
{
	for (int i = 0; i < LAYERS; i++)
		if (layers[i]->destroy)
			layers[i]->destroy();
}

bool
layers_init (void)
{
	struct layer *l[] = {
		layer_background(),
		layer_blanktile(),
		layer_osm(),
		layer_overview(),
		layer_cursor(),
	};

	// Copy to static list:
	memcpy(layers, l, sizeof(layers));

	for (int i = 0; i < LAYERS; i++)
		if (layers[i]->init)
			if (!layers[i]->init()) {
				layers_destroy();
				return false;
			}

	return true;
}
