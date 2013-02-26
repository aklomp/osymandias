#include <stdbool.h>
#include <stdlib.h>

struct layer {
	int zindex;
	struct layer *prev;
	struct layer *next;
	bool (*full_occlusion)(void);
	void (*paint)(void);
	void (*zoom)(void);
	void (*destroy)(void*);
	void *destroy_data;
};

static struct layer *layer_head = NULL;
static struct layer *layer_tail = NULL;

static void
layer_insert (struct layer *l)
{
	// Insert a layer into the linked list according to its z-index.
	struct layer *t;

	// If no layers yet, this one becomes the first:
	if (layer_head == NULL) {
		layer_head = layer_tail = l;
		return;
	}
	// Else, find the proper slot:
	for (t = layer_head; t; t = t->next) {
		if (t->zindex > l->zindex) {
			break;
		}
	}
	// Insert at end?
	if (t == NULL) {
		layer_tail->next = l;
		l->prev = layer_tail;
		layer_tail = l;
		return;
	}
	// Insert at head of the list?
	if (t->prev == NULL) {
		t->prev = l;
		l->next = t;
		layer_head = l;
		return;
	}
	// Insert before t:
	t->prev->next = l;
	l->prev = t->prev;
	l->next = t;
	t->prev = l;
}

static void
layer_destroy (struct layer **l)
{
	if (l == NULL || *l == NULL) {
		return;
	}
	if ((*l)->destroy) {
		(*l)->destroy((*l)->destroy_data);
	}
	free(*l);
	*l = NULL;
}

struct layer *
layer_create (bool (*full_occlusion)(void), void (*paint)(void), void (*zoom)(void), void (*destroy)(void *), void *destroy_data)
{
	struct layer *l;

	if ((l = malloc(sizeof(*l))) == NULL) {
		return NULL;
	}
	l->zindex = -1;
	l->prev = NULL;
	l->next = NULL;
	l->zoom = zoom;
	l->paint = paint;
	l->full_occlusion = full_occlusion;
	l->destroy = destroy;
	l->destroy_data = destroy_data;

	return l;
}

bool
layer_register (struct layer *l, int zindex)
{
	if (l == NULL) {
		return false;
	}
	l->zindex = zindex;
	layer_insert(l);
	return true;
}

void
layers_destroy (void)
{
	struct layer *t;

	for (struct layer *l = layer_head; l; l = t) {
		t = l->next;
		layer_destroy(&l);
	}
}

void
layers_paint (void)
{
	struct layer *layer;

	// Loop over all layers front to back, check if they occlude the background;
	// if they do, then don't bother painting lower layers:
	for (layer = layer_tail; layer; layer = layer->prev) {
		if (layer->full_occlusion()) {
			break;
		}
	}
	// If none of the layers claimed full occlusion, start at the head:
	if (layer == NULL) {
		layer = layer_head;
	}
	while (layer) {
		layer->paint();
		layer = layer->next;
	}
}

void
layers_zoom (void)
{
	for (struct layer *layer = layer_head; layer; layer = layer->next) {
		if (layer->zoom) {
			layer->zoom();
		}
	}
}
