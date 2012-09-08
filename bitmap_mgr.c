#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "pngloader.h"
#include "llist.h"

#define ZOOM_MAX	18
#define N_BITMAPS	1000	// Keep this many raw tiles in the cache

// Data structures.
// Each zoom level has a linked list of "x" rows. An "x" row knows which row it
// is from its .n member. Each "x" row contains a linked list of "y" tiles. A
// "y" tile also knows which tile column it's in. The "y" tile contains the
// actual pixel data. So to find a given tile:
//   1. Get the given x list from the zoom level;
//   2. traverse the x list until you find the proper x element;
//   3. get the y list from this x element;
//   4. traverse the y list until you find the tile.
// Of course, if you get a cache miss somewhere, you insert an "x" row or an
// "y" tile after the closest "smaller" sibling, and so on.

struct ytile {
	unsigned int n;
	struct list list;
	void *rawbits;
};

struct xlist {
	unsigned int n;
	struct list list;
	struct ytile *y;
};

struct zoomlevel {
	struct xlist *x;
};

static void *rawbits_get (const unsigned int zoom, const unsigned int xn, const unsigned int yn);

static struct xlist *find_closest_smaller_x (struct xlist *first, const unsigned int xn);
static struct ytile *find_closest_smaller_y (struct ytile *first, const unsigned int yn);

static struct xlist *create_xlist (const unsigned int zoom, struct xlist *closest, const unsigned int xn);
static struct ytile *create_ytile (struct xlist *x, struct ytile *closest, const unsigned int yn);

static void destroy_x_list (struct xlist *first);
static void destroy_y_list (struct ytile *first);

static struct zoomlevel *zoomlvl[ZOOM_MAX + 1];

static unsigned int tiles_cached = 0;

static char *
viking_filename (unsigned int zoom, int tile_x, int tile_y)
{
	int len;
	char *buf;
	char *home;

	if ((home = getenv("HOME")) == NULL) {
		return NULL;
	}
	if ((buf = malloc(100)) == NULL) {
		return NULL;
	}
	// FIXME: horrible copypaste of code from viewport.c:
	unsigned int world_size = ((unsigned int)1) << (zoom + 8);

	if ((len = snprintf(buf, 100, "%s/.viking-maps/t13s%uz0/%u/%u", home, 17 - zoom, tile_x, world_size / 256 - 1 - tile_y)) == 100) {
		free(buf);
		return NULL;
	}
	buf[len] = '\0';
	return buf;
}

static void *
bitmap_find_cached (const unsigned int zoom, const unsigned int xn, const unsigned int yn, struct xlist **x, struct ytile **y)
{
	// Walk the x and y lists, find the closest matching element. Ideally
	// the match is perfect, and we've found the cached tile. Otherwise the
	// match is the last smallest element (the one which our element should
	// come after), or NULL if the match was at the start of the list, or
	// the list is empty. This then becomes the new insertion point for a
	// procured xlist or ytile.
	if ((*x = find_closest_smaller_x(zoomlvl[zoom]->x, xn)) == NULL || (*x)->n != xn) {
		return NULL;
	}
	if ((*y = find_closest_smaller_y((*x)->y, yn)) == NULL || (*y)->n != yn) {
		return NULL;
	}
	return (*y)->rawbits;
}

void *
bitmap_request (const unsigned int zoom, const unsigned int xn, const unsigned int yn)
{
	void *s;
	struct xlist *xclosest = NULL;
	struct ytile *yclosest = NULL;

	// Do we already have the tile?
	if ((s = bitmap_find_cached(zoom, xn, yn, &xclosest, &yclosest)) != NULL) {
		return s;
	}
	// Do we at least have the right x col?
	if (xclosest == NULL || xclosest->n != xn) {
		// We don't, try to create it in place:
		if ((xclosest = create_xlist(zoom, xclosest, xn)) == NULL) {
			return NULL;
		}
	}
	// Here we know that have the proper x col (even if perhaps empty);
	// but we weren't able to find the tile before, so we know it does not exist:
	if ((yclosest = create_ytile(xclosest, yclosest, yn)) == NULL) {
		return NULL;
	}
	// Try to load the tile from file:
	return yclosest->rawbits = rawbits_get(zoom, xn, yn);
}

static void *
rawbits_get (const unsigned int zoom, const unsigned int xn, const unsigned int yn)
{
	char *filename;
	void *rawbits;
	unsigned int width;
	unsigned int height;

	if ((filename = viking_filename(zoom, xn, yn)) == NULL) {
		return NULL;
	}
	if (!load_png_file(filename, &height, &width, &rawbits)) {
		free(filename);
		return NULL;
	}
	free(filename);
	if (height != 256 || width != 256) {
		free(rawbits);
		return NULL;
	}
	if (rawbits != NULL) {
		tiles_cached++;
	}
	return rawbits;
}

static struct xlist *
find_closest_smaller_x (struct xlist *first, const unsigned int xn)
{
	struct xlist *x;

	list_foreach(first, x)
	{
		if (x->n < xn) {
			// Go on to next element, except if this was the last:
			if (list_next(x) == NULL) {
				break;
			}
			continue;
		}
		if (x->n == xn) return x;
		// This will be NULL if no previous member, and the
		// proper previous member if available:
		return list_prev(x);
	}
	// If we're here, x is either NULL or has an x->n smaller than xn;
	// in either case it's what we want:
	return x;
}

static struct ytile *
find_closest_smaller_y (struct ytile *first, const unsigned int yn)
{
	struct ytile *y;

	list_foreach(first, y)
	{
		if (y->n < yn) {
			if (list_next(y) == NULL) {
				break;
			}
			continue;
		}
		if (y->n == yn) return y;
		return list_prev(y);
	}
	return y;
}

static struct xlist *
create_xlist (const unsigned int zoom, struct xlist *closest, const unsigned int xn)
{
	struct xlist *list;

	if ((list = malloc(sizeof(*list))) == NULL) {
		return NULL;
	}
	list->n = xn;
	list->y = NULL;
	list_init(list);

	// No previous sibling?
	if (closest == NULL) {
		// No other elements in this list? This becomes the first:
		if (zoomlvl[zoom]->x == NULL) {
			zoomlvl[zoom]->x = list;
		}
		else {
			// Place before other elements in list, move head:
			list_insert_before(zoomlvl[zoom]->x, list);
			zoomlvl[zoom]->x = list;
		}
	}
	else {
		list_insert_after(closest, list);
	}
	return list;
}

static struct ytile *
create_ytile (struct xlist *x, struct ytile *closest, const unsigned int yn)
{
	struct ytile *tile;

	if ((tile = malloc(sizeof(*tile))) == NULL) {
		return NULL;
	}
	tile->n = yn;
	tile->rawbits = NULL;
	list_init(tile);

	// No previous sibling?
	if (closest == NULL) {
		// No other elements in this list? This becomes the first:
		if (x->y == NULL) {
			x->y = tile;
		}
		else {
			// Place before other elements in list, move head:
			list_insert_before(x->y, tile);
			x->y = tile;
		}
	}
	else {
		list_insert_after(closest, tile);
	}
	return tile;
}

static void
destroy_x_list (struct xlist *first)
{
	struct xlist *x;
	struct xlist *tx;

	list_foreach_safe(first, x, tx) {
		destroy_y_list(x->y);
		free(x);
	}
}

static void
destroy_y_list (struct ytile *first)
{
	struct ytile *y;
	struct ytile *ty;

	list_foreach_safe(first, y, ty) {
		free(y->rawbits);
		free(y);
	}
}

bool
bitmap_mgr_init (void)
{
	bool ret = false;

	for (int i = 0; i <= ZOOM_MAX; i++) zoomlvl[i] = NULL;

	// Allocate the zoom levels:
	for (int i = 0; i <= ZOOM_MAX; i++) {
		if ((zoomlvl[i] = malloc(sizeof(struct zoomlevel))) == NULL) {
			while (--i >= 0) {
				free(zoomlvl[i]);
			}
			goto err_0;
		}
		zoomlvl[i]->x = NULL;
	}
	ret = true;

err_0:	return ret;
}

void
bitmap_mgr_destroy (void)
{
	for (int i = 0; i <= ZOOM_MAX; i++) {
		destroy_x_list(zoomlvl[i]->x);
		free(zoomlvl[i]);
	}
}
