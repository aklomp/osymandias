#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "pngloader.h"
#include "llist.h"

#define ZOOM_MAX	18
#define MAX_BITMAPS	1000			// Keep at most this many bitmaps in the cache
#define PURGE_TO	(MAX_BITMAPS * 3 / 4)	// Don't stop purging till this much left

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

static unsigned int destroy_x_list (struct xlist *first);
static unsigned int destroy_y_list (struct ytile *first);

static void cache_purge (const unsigned int zoom, const unsigned int xn, const unsigned int yn);

static struct zoomlevel *zoomlvl[ZOOM_MAX + 1];

static unsigned int tiles_cached = 0;

static struct xlist *last_closest_x = NULL;
static struct ytile *last_closest_y = NULL;
static unsigned int last_zoom = 0;

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
	// These are the naieve starting values for the x and y starting
	// positions. Start searching for the x col from the start of the list,
	// and the y col from whatever we find for the x col.
	struct xlist *start_x = zoomlvl[zoom]->x;
	struct ytile *start_y = NULL;

	// Walk the x and y lists, find the closest matching element. Ideally
	// the match is perfect, and we've found the cached tile. Otherwise the
	// match is the last smallest element (the one which our element should
	// come after), or NULL if the match was at the start of the list, or
	// the list is empty. This then becomes the new insertion point for a
	// procured xlist or ytile.

	// Go to work with the cached (static) values of last_zoom,
	// last_closest_x, and last_closest_y. The thinking is that this
	// function is called in a loop with sequential x and y coordinates.
	// Why not speed things up by searching from the last tile instead of
	// searching from the start of the linked list? Ideally we'd find the
	// next tile straight away.

	if (zoom != last_zoom) {
		// Don't use any cached values:
		last_zoom = zoom;
	}
	else if (last_closest_x != NULL) {
		// If the last xn was the same as the current xn, then we're on
		// the right col and can set search_y to something other than
		// NULL. We use the fact that search_y is not NULL later on to
		// skip the search for the x col entirely.
		if (last_closest_x->n == xn) {
			start_x = last_closest_x;
			if (last_closest_y != NULL) {
				if (last_closest_y->n <= yn) {
					// Start searching from this node:
					start_y = last_closest_y;
				}
				else if (last_closest_y->n < yn + 20) {
					// Last tile was close to this one, but has a slightly larger index.
					// Backtrack a bit, see if we get a hit or a smaller sibling:
					int i;
					struct ytile *tmp = last_closest_y;
					for (i = 0; i < 20; i++) {
						if (tmp->n <= yn) {
							start_y = tmp;
							break;
						}
						if ((tmp = list_prev(tmp)) == NULL) {
							break;
						}
					}
				}
			}
		}
		// If last_closest_x->n is smaller than xn, we can start searching from
		// there instead of from the start of the list.
		else if (last_closest_x->n < xn) {
			start_x = last_closest_x;
		}
		else if (last_closest_x->n < xn + 20) {
			// Run over the list backwards for max 20 hops, see if we can find a starting point:
			int i;
			struct xlist *tmp = last_closest_x;
			for (i = 0; i < 20; i++) {
				if (tmp->n <= xn) {
					start_x = tmp;
					break;
				}
				if ((tmp = list_prev(tmp)) == NULL) {
					break;
				}
			}
		}
	}
	// If start_y is not NULL, we know we're already on the right x row and can skip the search;
	// else we start searching from start_x, which may or may not be close to the target:
	if (start_y == NULL) {
		if ((*x = last_closest_x = find_closest_smaller_x(start_x, xn)) == NULL || (*x)->n != xn) {
			return NULL;
		}
		start_y = (*x)->y;
	}
	else {
		// Ensure the variable is always updated for the caller:
		*x = last_closest_x;
	}
	if ((*y = last_closest_y = find_closest_smaller_y(start_y, yn)) == NULL || (*y)->n != yn) {
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
	struct xlist *xmatch;
	struct ytile *ymatch;

	// Do we already have the tile?
	if ((s = bitmap_find_cached(zoom, xn, yn, &xclosest, &yclosest)) != NULL) {
		return s;
	}
	// Do we at least have the right x col?
	if (xclosest == NULL || xclosest->n != xn) {
		// We don't, try to create it in place:
		if ((xmatch = create_xlist(zoom, xclosest, xn)) == NULL) {
			return NULL;
		}
	}
	else {
		// Yes we do: exact match:
		xmatch = xclosest;
	}
	// Here we know that have the proper x col (even if perhaps empty);
	// but we weren't able to find the tile before, so we know it does not exist:
	if ((ymatch = create_ytile(xmatch, yclosest, yn)) == NULL) {
		return NULL;
	}
	// Should we purge the cache first, before loading new images?
	if (tiles_cached >= MAX_BITMAPS) {
		cache_purge(zoom, xn, yn);
	}
	// Try to load the tile from file:
	if ((ymatch->rawbits = rawbits_get(zoom, xn, yn)) != NULL) {
		return ymatch->rawbits;
	}
	// If the lookup failed, do not hang an empty tile in the list:
	list_detach(xmatch->y, ymatch);
	free(ymatch);
	return NULL;
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

static unsigned int
destroy_x_list (struct xlist *first)
{
	struct xlist *x;
	struct xlist *tx;
	unsigned int nfreed = 0;

	list_foreach_safe(first, x, tx) {
		nfreed += destroy_y_list(x->y);
		free(x);
	}
	return nfreed;
}

static unsigned int
destroy_y_list (struct ytile *first)
{
	struct ytile *y;
	struct ytile *ty;
	unsigned int nfreed = 0;

	// nfreed only counts *rawbits* freed; the data structure
	// itself is considered "free" in terms of memory:
	list_foreach_safe(first, y, ty) {
		if (y->rawbits != NULL) {
			free(y->rawbits);
			nfreed++;
		}
		free(y);
	}
	return nfreed;
}

static void
cache_purge (const unsigned int zoom, const unsigned int xn, const unsigned int yn)
{
	// Purge items from the cache till only #PURGE_TO tiles left.
	// Use a heuristic method to remove "unimportant" tiles first.

	// First, they came for the far higher zoom levels:
	if (zoom + 3 <= ZOOM_MAX) {
		for (unsigned int i = ZOOM_MAX; i > zoom + 2; i--) {
			tiles_cached -= destroy_x_list(zoomlvl[i]->x);
			zoomlvl[i]->x = NULL;
			if (tiles_cached <= PURGE_TO) {
				return;
			}
		}
	}
	// Then, they came for the far lower zoom levels:
	if (zoom > 3) {
		for (unsigned int i = 0; i < zoom - 3; i++) {
			tiles_cached -= destroy_x_list(zoomlvl[i]->x);
			zoomlvl[i]->x = NULL;
			if (tiles_cached <= PURGE_TO) {
				return;
			}
		}
	}
	// Then, they came for the directly higher zoom levels:
	for (unsigned int i = ZOOM_MAX; i > zoom; i--) {
		tiles_cached -= destroy_x_list(zoomlvl[i]->x);
		zoomlvl[i]->x = NULL;
		if (tiles_cached <= PURGE_TO) {
			return;
		}
	}
	// Then, they came for the directly lower zoom levels:
	for (unsigned int i = 0; i < zoom; i++) {
		tiles_cached -= destroy_x_list(zoomlvl[i]->x);
		zoomlvl[i]->x = NULL;
		if (tiles_cached <= PURGE_TO) {
			return;
		}
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
