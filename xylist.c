#include <stdbool.h>
#include <stdlib.h>

#include "llist.h"
#include "xylist.h"

// An xylist is a kind of linked list that looks like this:
//
// zoomlevel ->  x -> x -> x -> x -> x -> x -> x -> x
//               |    |    |    |    |    |    |    |
//               y    y    y    y    y    y    y    y
//               |    |    |         |    |    |
//               y    y    y         y    y    y
//                    |    |              |
//                    y    y              y
//
// To find a tile at (xn,yn), first traverse the x linked list, and when you
// find the element with x->n == xn, traverse its y linked list until you find
// the element with y->n == yn. The "tile" itself is a void pointer to an opaque
// data structure (private to caller).
//
// This xylist data structure lets us insert new tiles quickly at a given
// coordinate. It also lets us remove "unused" tiles quickly by purging zoom
// levels, x rows and y elements that are out of view.
//
// When creating an xylist, the caller supplies a pointer to a callback function
// that can procure a data pointer for (zoom, xn, yn). When a tile is requested,
// and it cannot be found in the list, the callback is called to procure the
// tile data. When the cache cleanup runs, the destructor callback is called to
// flush the data. This allows the xylist to be "generic", it deals with storing
// opaque pointers at a (zoom,x,y) coordinate. In a sense it's a kind of database.
//
// When we want a set of tiles to cover the screen, we start at the first y at
// first x, and end at the last y at the last x. By saving the x and y pointers
// used for the last lookup, and using them at the next lookup (checking that
// they are appropriate, of course), we can retrieve elements really fast,
// almost as fast as a sequential lookup.

struct ytile {
	unsigned int n;
	void *data;
	struct list list;
};

struct xlist {
	unsigned int n;
	struct ytile *y;
	struct list list;
};

struct xylist {
	unsigned int zoom_min;
	unsigned int zoom_max;
	unsigned int tiles_max;
	unsigned int tiles_cur;
	struct xlist **x;
	struct xlist **cached_x;
	struct ytile **cached_y;
	void *(*tile_procure)(unsigned int zoom, unsigned int xn, unsigned int yn, unsigned int search_depth);
	void  (*tile_destroy)(void *data);
};

// Number of tiles to purge the cache to when the max number of tiles is reached:
#define PURGE_TO(l)	((l)->tiles_max * 3 / 4)

// Return pointer to x pointer at zoom level:
#define HEAD_X(l,z)	((l)->x + ((z) - (l)->zoom_min))

// Return pointer to "last x" pointer at zoom level:
#define CACHED_X(l,z)	((l)->cached_x + ((z) - (l)->zoom_min))

// Return pointer to "last y" pointer at zoom level:
#define CACHED_Y(l,z)	((l)->cached_y + ((z) - (l)->zoom_min))

static void
ytile_destroy (struct xylist *l, struct ytile **y, const unsigned int zoom, const unsigned int xn)
{
	struct xlist **cached_x = CACHED_X(l, zoom);
	struct ytile **cached_y;

	if (y == NULL || *y == NULL) {
		return;
	}
	// Does the cached pointer point to here?
	if (*cached_x && (*cached_x)->n == xn) {
		cached_y = CACHED_Y(l, zoom);
		if (*cached_y == *y) {
			*cached_x = NULL;
			*cached_y = NULL;
		}
	}
	// Call custom destructor on data pointer:
	l->tile_destroy((*y)->data);
	l->tiles_cur--;
	free(*y);
	*y = NULL;
}

static void
xlist_destroy (struct xylist *l, struct xlist **x, const unsigned int zoom)
{
	struct ytile *y = (*x)->y;

	// Node must be detached by caller:
	while (y) {
		struct ytile *yt = list_next(y);
		list_detach(&(*x)->y, y);
		ytile_destroy(l, &y, zoom, (*x)->n);
		y = yt;
	}
	free(*x);
	*x = NULL;
}

static void
zoomlevel_destroy (struct xylist *l, const unsigned int zoom)
{
	struct xlist **anchor = HEAD_X(l, zoom);
	struct xlist *x = *anchor;

	if (zoom < l->zoom_min || zoom > l->zoom_max) {
		return;
	}
	while (x) {
		struct xlist *xt = list_next(x);
		struct ytile *y = x->y;
		while (y) {
			struct ytile *yt = list_next(y);
			list_detach(&x->y, y);
			ytile_destroy(l, &y, zoom, x->n);
			y = yt;
		}
		free(x);
		x = xt;
	}
	*anchor = NULL;
	return;
}

struct xylist *
xylist_create (const unsigned int zoom_min, const unsigned int zoom_max, const unsigned int tiles_max,
		void *(*callback_procure)(unsigned int zoom, unsigned int xn, unsigned int yn, const unsigned int search_depth),
		void  (*callback_destroy)(void *data))
{
	struct xylist *l;
	unsigned int n_zoomlevels = zoom_max + 1 - zoom_min;

	if ((l = malloc(sizeof(*l))) == NULL) {
		goto err_0;
	}
	if ((l->x = calloc(n_zoomlevels, sizeof(struct xlist *))) == NULL) {
		goto err_1;
	}
	if ((l->cached_x = calloc(n_zoomlevels, sizeof(struct xlist *))) == NULL) {
		goto err_2;
	}
	if ((l->cached_y = calloc(n_zoomlevels, sizeof(struct ytile *))) == NULL) {
		goto err_3;
	}
	l->zoom_min = zoom_min;
	l->zoom_max = zoom_max;
	l->tiles_cur = 0;
	l->tiles_max = tiles_max;
	l->tile_procure = callback_procure;
	l->tile_destroy = callback_destroy;

	// Initialize pointer tables:
	for (unsigned int z = l->zoom_min; z <= l->zoom_max; z++) {
		*HEAD_X(l, z) = NULL;
		*CACHED_X(l, z) = NULL;
		*CACHED_Y(l, z) = NULL;
	}
	return l;

err_3:	free(l->cached_x);
err_2:	free(l->x);
err_1:	free(l);
err_0:	return NULL;
}

void
xylist_destroy (struct xylist **l)
{
	unsigned int z;

	if (l == NULL || *l == NULL) {
		return;
	}
	for (z = (*l)->zoom_min; z <= (*l)->zoom_max; z++) {
		zoomlevel_destroy(*l, z);
	}
	free((*l)->cached_y);
	free((*l)->cached_x);
	free((*l)->x);
	free(*l);
	*l = NULL;
}

static struct xlist *
find_closest_smaller_x (struct xlist *startfrom, const unsigned int xn)
{
	struct xlist *x;

	// Starting at startfrom (which can be NULL), find the xlist with the
	// closest smaller index number compared to xn. A "distance" of zero
	// (i.e. an exact match) also counts. This returns either the desired
	// element, or the element after which to insert a new xlist.
	// A return value of NULL means "insert at start".

	list_foreach(startfrom, x) {
		// If index is smaller than the one we want, continue;
		// except when this is the last list element:
		if (x->n < xn) {
			if (list_next(x) == NULL) {
				return x;
			}
			continue;
		}
		// If exact match, we're done:
		else if (x->n == xn) {
			return x;
		}
		// Passed the target; return the previous element
		// (which can be NULL):
		else return list_prev(x);
	}
	// If we're here, the list_foreach never ran (empty list):
	return NULL;
}

static struct ytile *
find_closest_smaller_y (struct ytile *startfrom, const unsigned int yn)
{
	struct ytile *y;

	// For explanation, see equivalent function for xlists above.
	list_foreach(startfrom, y) {
		if (y->n < yn) {
			if (list_next(y) == NULL) {
				return y;
			}
			continue;
		}
		else if (y->n == yn) {
			return y;
		}
		else return list_prev(y);
	}
	return y;
}

static void *
tile_find_cached (struct xylist *l, const unsigned int zoom, const unsigned int xn, const unsigned int yn, struct xlist **xclosest, struct ytile **yclosest)
{
	unsigned int i;
	struct xlist *start_x = *CACHED_X(l, zoom);
	struct ytile *start_y = NULL;

	// Walk the x and y lists, find the closest matching element. Ideally
	// the match is perfect, and we've found the cached tile. Otherwise the
	// match is the last smallest element (the one which our element should
	// come after), or NULL if the match was at the start of the list, or
	// the list is empty. This then becomes the new insertion point for a
	// procured xlist or ytile.

	// Go to work with the cached values of start_x and start_y. The
	// thinking is that this function is called in a loop with sequential x
	// and y coordinates. Why not speed things up by searching from the
	// last tile instead of searching from the start of the linked list?
	// Ideally we'd find the next tile straight away.

	// If no last starting position, start at head:
	if (start_x == NULL) {
		start_x = *HEAD_X(l, zoom);
		goto search;
	}
	// Smaller than desired index? Viable starting point:
	if (start_x->n < xn) {
		goto search;
	}
	// Exact match? Try to close in on the y start:
	if (start_x->n == xn)
	{
		// No good previous value to work from?
		if ((start_y = *CACHED_Y(l, zoom)) == NULL) {
			goto search;
		}
		// Smaller or equal to desired index?
		if (start_y->n <= yn) {
			goto search;
		}
		// Only slightly larger? Backtrack a little:
		if (start_y->n < yn + 10) {
			struct ytile *y = start_y;
			for (i = 0; i < 10; i++) {
				if (y->n <= yn) {
					start_y = y;
					goto search;
				}
				if ((y = list_prev(y)) == NULL) {
					break;
				}
			}
		}
		start_y = NULL;
		goto search;
	}
	// Only slightly larger? Backtrack a little:
	if (start_x->n < xn + 10) {
		struct xlist *x = start_x;
		for (i = 0; i < 10; i++) {
			if (x->n <= xn) {
				start_x = x;
				goto search;
			}
			if ((x = list_prev(x)) == NULL) {
				break;
			}
		}
		// FALLTHROUGH
	}
	// Must be much larger; give up and start at head:
	start_x = *HEAD_X(l, zoom);

search:	// If not already on right x col, we must find it first:
	if (start_y == NULL || start_x->n != xn) {
		*xclosest = find_closest_smaller_x(start_x, xn);
		if (*xclosest == NULL || (*xclosest)->n != xn) {
			return NULL;
		}
		start_y = (*xclosest)->y;
	}
	else {
		// Ensure the variable is always updated for the caller:
		*xclosest = start_x;
	}
	*yclosest = find_closest_smaller_y(start_y, yn);
	if (*yclosest == NULL || (*yclosest)->n < yn) {
		return NULL;
	}
	// Found tile, update lookup cache data:
	*CACHED_X(l, zoom) = *xclosest;
	*CACHED_Y(l, zoom) = *yclosest;
	return (*yclosest)->data;
}

bool
xylist_area_is_covered (struct xylist *l, const unsigned int zoom, const unsigned int tile_xmin, const unsigned int tile_ymin, const unsigned int tile_xmax, const unsigned int tile_ymax)
{
	bool ret = false;
	struct xlist **cached_x = CACHED_X(l, zoom);
	struct ytile **cached_y = CACHED_Y(l, zoom);
	struct xlist *current_cached_x = *cached_x;
	struct ytile *current_cached_y = *cached_y;
	struct xlist *xclosest = NULL;
	struct ytile *yclosest = NULL;

	// Return true if we can guarantee coverage of the area given by the tile
	// coordinates, and false if we're not positive. Store the cached pointers
	// and restore them at the end, ready for the next walk over the area.
	for (unsigned int xn = tile_xmin; xn <= tile_xmax; xn++) {
		for (unsigned int yn = tile_ymin; yn <= tile_ymax; yn++) {
			if (tile_find_cached(l, zoom, xn, yn, &xclosest, &yclosest) == NULL) {
				goto out;
			}
		}
	}
	ret = true;

out:	*cached_x = current_cached_x;
	*cached_y = current_cached_y;
	return ret;
}

static struct xlist *
xlist_insert (struct xylist *l, struct xlist *after, const unsigned int zoom, const unsigned int xn)
{
	struct xlist *x;

	if ((x = malloc(sizeof(*x))) == NULL) {
		return NULL;
	}
	x->n = xn;
	x->y = NULL;
	list_init(x);

	// No previous sibling?
	if (after == NULL) {
		struct xlist **head = HEAD_X(l, zoom);
		// If other elements in list, put this in front:
		if (*head != NULL) {
			list_insert_before(*head, x);
		}
		*head = x;
	}
	else {
		list_insert_after(after, x);
	}
	return x;
}

static struct ytile *
ytile_insert (struct xylist *l, struct xlist *x, struct ytile *after, const unsigned int yn, void *const data)
{
	struct ytile *y;

	if ((y = malloc(sizeof(*y))) == NULL) {
		return NULL;
	}
	y->n = yn;
	y->data = data;
	list_init(y);
	l->tiles_cur++;

	// No previous sibling?
	if (after == NULL) {
		// If other elements in list, put this in front:
		if (x->y != NULL) {
			list_insert_before(x->y, y);
		}
		x->y = y;
	}	
	else {
		list_insert_after(after, y);
	}
	return y;
}

static void
cache_purge_x_cols (struct xylist *l, const unsigned int zoom, const unsigned int xn)
{
	unsigned int purge_to = PURGE_TO(l);
	struct xlist **anchor = HEAD_X(l, zoom);
	struct xlist *xs = *anchor;
	struct xlist *xe;

	list_last(xs, xe);

	while (*anchor)
	{
		// Keep a pointer for the start and end of the list;
		// we will destroy the node that is the furthest from us:
		unsigned int sdist = xn - xs->n;
		unsigned int edist = xe->n - xn;

		// Both start and end cols too close to target?
		if (sdist + 5 > xs->n || xe->n + 5 < edist) {
			break;
		}
		// If distance to start is largest, delete that one:
		if (sdist >= edist) {
			struct xlist *xt = list_next(xs);
			list_detach(anchor, xs);
			xlist_destroy(l, &xs, zoom);
			if ((xs = xt) == NULL) {
				break;
			}
		}
		// Else delete the end tile:
		else {
			struct xlist *xt = list_prev(xe);
			list_detach(anchor, xe);
			xlist_destroy(l, &xe, zoom);
			if ((xe = xt) == NULL) {
				break;
			}
		}
		if (*anchor == NULL) {
			break;
		}
		if (l->tiles_cur <= purge_to) {
			break;
		}
	}
}

static void
cache_purge_y_rows (struct xylist *l, const unsigned int zoom, const unsigned int yn)
{
	unsigned int purge_to = PURGE_TO(l);
	struct xlist **anchor = HEAD_X(l, zoom);
	struct xlist *x = *anchor;

	// See cache_purge_x_cols for explanation.
	// Basically we delete the y tiles which are furthest from current.

	while (x)
	{
		struct xlist *xt = list_next(x);
		struct ytile *ys = x->y;
		struct ytile *ye;

		list_last(ys, ye);

		while (x->y)
		{
			unsigned int sdist = yn - ys->n;
			unsigned int edist = ye->n - yn;

			if (sdist + 5 > ys->n || ye->n + 5 < edist) {
				break;
			}
			if (sdist >= edist) {
				struct ytile *yt = list_next(ys);
				list_detach(&x->y, ys);
				ytile_destroy(l, &ys, zoom, x->n);
				if ((ys = yt) == NULL) {
					break;
				}
			}
			else {
				struct ytile *yt = list_prev(ye);
				list_detach(&x->y, ye);
				ytile_destroy(l, &ye, zoom, x->n);
				if ((ye = yt) == NULL) {
					break;
				}
			}
			if (l->tiles_cur <= purge_to) {
				break;
			}
		}
		if (x->y == NULL) {
			list_detach(anchor, x);
			xlist_destroy(l, &x, zoom);
			break;
		}
		x = xt;
	}
}

static void
cache_purge (struct xylist *l, const unsigned int zoom, const unsigned int xn, const unsigned int yn)
{
	unsigned int z;
	unsigned int purge_to = PURGE_TO(l);

	// Purge items from the cache till only #PURGE_TO() tiles left.
	// Use a heuristic method to remove "unimportant" tiles first.

	// First, they came for the far higher zoom levels:
	for (z = l->zoom_max; z > zoom + 2; z--) {
		zoomlevel_destroy(l, z);
		if (l->tiles_cur <= purge_to) {
			return;
		}
	}
	// Then, they came for the far lower zoom levels:
	if (zoom > l->zoom_min + 2) {
		for (z = l->zoom_min; z < zoom - 2; z++) {
			zoomlevel_destroy(l, z);
			if (l->tiles_cur <= purge_to) {
				return;
			}
		}
	}
	// Then, they came for the directly higher zoom levels:
	for (z = l->zoom_max; z > zoom; z--) {
		zoomlevel_destroy(l, z);
		if (l->tiles_cur <= purge_to) {
			return;
		}
	}
	// Then, they came for the directly lower zoom levels:
	for (z = l->zoom_min; z < zoom; z++) {
		zoomlevel_destroy(l, z);
		if (l->tiles_cur <= purge_to) {
			return;
		}
	}
	// Then, they got the start and end of the x columns, and deleted the furthest columns:
	cache_purge_x_cols(l, zoom, xn);
	if (l->tiles_cur <= purge_to) {
		return;
	}
	// Then, finally, they went for the y columns:
	cache_purge_y_rows(l, zoom, yn);

	// And then there was nothing more to come for...
	zoomlevel_destroy(l, zoom);
}

void *
xylist_request (struct xylist *l, const unsigned int zoom, const unsigned int xn, const unsigned int yn, unsigned int search_depth)
{
	void *data;
	struct xlist *xclosest = NULL;
	struct ytile *yclosest = NULL;
	struct xlist *xmatch;
	struct ytile *ymatch;

	// Is the request invalid or out of bounds?
	// TODO: for efficiency, perhaps remove this check:
	if (l == NULL || zoom < l->zoom_min || zoom > l->zoom_max) {
		return NULL;
	}
	// Can we find the tile in the cache?
start:	if ((data = tile_find_cached(l, zoom, xn, yn, &xclosest, &yclosest)) != NULL) {
		return data;
	}
	if (search_depth == 0) {
		return NULL;
	}
	// Can't fetch the tile from the cache; try to make a new tile.
	// Purge items from the cache if it's getting too full:
	if (l->tiles_cur >= l->tiles_max) {
		cache_purge(l, zoom, xn, yn);
		if (l->tiles_cur >= PURGE_TO(l)) {
			return NULL;
		}
		// At this point we cannot trust xclosest and yclosest any more; regenerate:
		xclosest = NULL;
		yclosest = NULL;
		goto start;
	}
	// issue a callback to procure the tile data; if this fails, we're sunk:
	if ((data = l->tile_procure(zoom, xn, yn, search_depth - 1)) == NULL) {
		return NULL;
	}
	// New data; hang it in to the xylist after the closest x and y members:
	// Do we already have the right x col?
	if (xclosest == NULL || xclosest->n != xn) {
		// Nope, try to create it in place:
		if ((xmatch = xlist_insert(l, xclosest, zoom, xn)) == NULL) {
			// This should never happen, but if so, destroy fresh tile:
			l->tile_destroy(data);
			return NULL;
		}
	}
	else {
		// Yes we do: exact match:
		xmatch = xclosest;
	}
	// Here we know that have the proper x col (even if perhaps empty);
	// but we weren't able to find the tile before, so we know it does not exist:
	if ((ymatch = ytile_insert(l, xmatch, yclosest, yn, data)) == NULL) {
		// Again, this should not happen; trash our freshly procured data:
		l->tile_destroy(data);
		return NULL;
	}
	// Update lookup cache values:
	*CACHED_X(l, zoom) = xmatch;
	*CACHED_Y(l, zoom) = ymatch;
	return data;
}
