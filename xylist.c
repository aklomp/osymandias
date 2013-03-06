#include <stdbool.h>
#include <stdlib.h>

#include "llist.h"
#include "xylist.h"

// An xylist is a kind of doubly linked list that looks like this:
//
//                           |ys
// zoomlevel -> xs > x = x = x = x = x = x = x = x < xe
//                   |   |   |   |   |   |   |   |
//                   y   y   y   y   y   y   y   y
//                   |   |   |       |   |   |
//                   y   y   y       y   y   y
//                       |   |               |
//                       y   y               y
//                           |ye
//
// To find a tile at (xn,yn), first traverse the x linked list, and when you
// find the element with x->n == xn, traverse its y linked list until you find
// the element with y->n == yn. The "tile" itself is a void pointer to an
// opaque data structure (private to caller). The list has two ends: xs/ys,
// which is the head of the list, and xe/ye, which is the tail. So we can start
// looking from the most promising end.
//
// This xylist data structure lets us insert new tiles quickly at a given
// coordinate. It also lets us remove "unused" tiles quickly by purging zoom
// levels, x rows and y elements that are out of view.
//
// When creating an xylist, the caller supplies a pointer to a callback
// function that can procure a data pointer for (zoom, xn, yn). When a tile is
// requested, and it cannot be found in the list, the callback is called to
// procure the tile data. When the cache cleanup runs, the destructor callback
// is called to flush the data. This allows the xylist to be "generic", it
// deals with storing opaque pointers at a (zoom,x,y) coordinate. In a sense
// it's a kind of database.
//
// When we want a set of tiles to cover the screen, we start at the first y at
// first x, and end at the last y at the last x. By saving the x and y pointers
// used for the last lookup, and using them at the next lookup (checking that
// they are appropriate, of course), we can retrieve elements really fast,
// almost as fast as a sequential lookup.

struct ytile {
	void *data;		// opaque data pointer
	unsigned int n;		// row number
	struct list list;
};

struct xlist {
	unsigned int n;		// col number
	struct ytile *ys;	// y list start
	struct ytile *ye;	// y list end
	struct list list;
};

struct zoomlevel {
	unsigned int n;		// zoom level number
	struct xlist *xs;	// x list start
	struct xlist *xe;	// x list end
	unsigned int ntiles;
	struct xlist *saved_x;
	struct ytile *saved_y;
};

struct xylist {
	unsigned int zoom_min;
	unsigned int zoom_max;
	unsigned int tiles_max;
	unsigned int purge_to;
	unsigned int ntiles;
	struct zoomlevel **zoom;
	void *(*tile_procure)(struct xylist_req *req);
	void  (*tile_destroy)(void *data);
};

static void
ytile_destroy (struct xylist *l, struct zoomlevel *z, struct ytile **yy, const unsigned int xn)
{
	struct ytile *y = *yy;

	if (y == NULL) {
		return;
	}
	// Does the cached pointer point to here?
	if (z->saved_x != NULL && z->saved_x->n == xn) {
		if (z->saved_y != NULL && z->saved_y == y) {
			z->saved_x = NULL;
			z->saved_y = NULL;
		}
	}
	// Call custom destructor on data pointer:
	l->tile_destroy(y->data);
	l->ntiles--;
	z->ntiles--;
	free(y);
	*yy = NULL;
}

static void
xlist_destroy (struct xylist *l, struct zoomlevel *z, struct xlist **xx)
{
	struct xlist *x = *xx;
	struct ytile *y = x->ys;

	// Node must be detached by caller.
	if (z->saved_x && z->saved_x->n == x->n) {
		z->saved_x = NULL;
		z->saved_y = NULL;
	}
	while (y) {
		// Optimize this: we know we are trashing the y list,
		// so don't bother to detach and be nice:
		struct ytile *yt = list_next(y);
		l->tile_destroy(y->data);
		l->ntiles--;
		z->ntiles--;
		free(y);
		y = yt;
	}
	free(x);
	*xx = NULL;
}

static void
cache_purge_zoomlevel (struct xylist *l, struct zoomlevel *z)
{
	struct xlist *x;

	if (z->ntiles == 0) {
		return;
	}
	x = z->xs;
	while (x) {
		// Don't bother to detach the x list properly,
		// we're trashing the entire zoom level anyway:
		struct xlist *xt = list_next(x);
		xlist_destroy(l, z, &x);
		x = xt;
	}
	z->xs = NULL;
	z->xe = NULL;
	z->ntiles = 0;
	z->saved_x = NULL;
	z->saved_y = NULL;
}

void
xylist_purge_other_zoomlevels (struct xylist *l, const unsigned int zoom)
{
	for (unsigned int z = l->zoom_min; z <= l->zoom_max; z++) {
		if (z == zoom) {
			continue;
		}
		cache_purge_zoomlevel(l, l->zoom[z]);
	}
}

static struct zoomlevel *
zoomlevel_create (const unsigned int level)
{
	struct zoomlevel *z;

	if ((z = malloc(sizeof(*z))) == NULL) {
		return NULL;
	}
	z->xs = NULL;
	z->xe = NULL;
	z->n = level;
	z->ntiles = 0;
	z->saved_x = NULL;
	z->saved_y = NULL;
	return z;
}

static void
zoomlevel_destroy (struct xylist *l, struct zoomlevel **z)
{
	// First empty the zoomlevel:
	cache_purge_zoomlevel(l, *z);

	free(*z);
	*z = NULL;
}

struct xylist *
xylist_create (const unsigned int zoom_min, const unsigned int zoom_max, const unsigned int tiles_max,
		void *(*callback_procure)(struct xylist_req *req),
		void  (*callback_destroy)(void *data))
{
	struct xylist *l;

	if ((l = malloc(sizeof(*l))) == NULL) {
		goto err_0;
	}
	if ((l->zoom = calloc(19, sizeof(*l->zoom))) == NULL) {
		goto err_1;
	}
	l->zoom_min = zoom_min;
	l->zoom_max = zoom_max;
	l->ntiles = 0;
	l->tiles_max = tiles_max;
	l->purge_to = tiles_max * 3 / 4;
	l->tile_procure = callback_procure;
	l->tile_destroy = callback_destroy;

	// Initialize zoom levels:
	for (int z = 0; z <= 18; z++) {
		if ((l->zoom[z] = zoomlevel_create(z)) == NULL) {
			for (int i = z - 1; i >= 0; i--) {
				free(l->zoom[i]);
			}
			goto err_1;
		}
	}
	return l;

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
	for (z = 0; z <= 18; z++) {
		zoomlevel_destroy(*l, &((*l)->zoom[z]));
	}
	free((*l)->zoom);
	free(*l);
	*l = NULL;
}

static struct xlist *
find_closest_smaller_x (struct xlist *startfrom, const unsigned int xn)
{
	struct xlist *x = startfrom;
	struct xlist *xt;

	// Starting at startfrom (which can be NULL), find the xlist with the
	// closest smaller index number compared to xn. A "distance" of zero
	// (i.e. an exact match) also counts. This returns either the desired
	// element, or the element after which to insert a new xlist.
	// A return value of NULL means "insert at start".

	while (x) {
		// If index is smaller than the one we want, continue;
		// except when this is the last list element:
		if (x->n < xn) {
			if ((xt = list_next(x)) == NULL) {
				return x;
			}
			x = xt;
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
	struct ytile *y = startfrom;
	struct ytile *yt;

	// For explanation, see equivalent function for xlists above.
	while (y) {
		if (y->n < yn) {
			if ((yt = list_next(y)) == NULL) {
				return y;
			}
			y = yt;
			continue;
		}
		else if (y->n == yn) {
			return y;
		}
		else return list_prev(y);
	}
	return NULL;
}

static struct xlist *
find_closest_smaller_x_reverse (struct xlist *startfrom, const unsigned int xn)
{
	// Like find_closest_smaller_x(), but starts at end and works to front.
	struct xlist *x = startfrom;

	while (x) {
		if (x->n > xn) {
			x = list_prev(x);
			continue;
		}
		return x;
	}
	return NULL;
}

static struct ytile *
find_closest_smaller_y_reverse (struct ytile *startfrom, const unsigned int yn)
{
	struct ytile *y = startfrom;

	while (y) {
		if (y->n > yn) {
			y = list_prev(y);
			continue;
		}
		return y;
	}
	return NULL;
}

static void *
tile_find_cached (struct zoomlevel *z, const unsigned int xn, const unsigned int yn, struct xlist **xclosest, struct ytile **yclosest)
{
	unsigned int i;
	unsigned int reverse_x = 0;
	unsigned int reverse_y = 0;
	struct xlist *start_x = z->saved_x;
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

	if (start_x == NULL) {
		goto set_x;
	}
	// Smaller than desired index? Viable starting point:
	if (start_x->n < xn) {
		goto search;
	}
	// Exact match? Try to close in on the y start:
	if (start_x->n == xn)
	{
		// No good previous value to work from?
		if ((start_y = z->saved_y) == NULL) {
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
	// Must be much larger; give up and start at most likely candidate:

set_x:	// If no last starting position, and distance from head
	// looks smaller, start at head:
	if (z->xs == NULL) {
		*yclosest = NULL;
		*xclosest = NULL;
		return NULL;
	}
	if (xn - z->xs->n <= z->xe->n - xn) {
		start_x = z->xs;
	}
	// Else start at end and go in reverse:
	else {
		start_x = z->xe;
		reverse_x = 1;
	}

search:	// If not already on right x col, we must find it first:
	if (start_y == NULL || start_x->n != xn) {
		struct xlist *xt;
		if (reverse_x) {
			xt = find_closest_smaller_x_reverse(start_x, xn);
		}
		else {
			xt = find_closest_smaller_x(start_x, xn);
		}
		if (xt == NULL || xt->n != xn || xt->ys == NULL) {
			*xclosest = xt;
			*yclosest = NULL;
			return NULL;
		}
		// For starting position of y, use list head or list tail,
		// whichever is closer:
		if (yn - xt->ys->n <= xt->ye->n - yn) {
			start_y = xt->ys;
		}
		else {
			start_y = xt->ye;
			reverse_y = 1;
		}
		*xclosest = xt;
	}
	else {
		// Ensure the variable is always updated for the caller:
		*xclosest = start_x;
	}
	if (reverse_y) {
		*yclosest = find_closest_smaller_y_reverse(start_y, yn);
	}
	else {
		*yclosest = find_closest_smaller_y(start_y, yn);
	}
	if (*yclosest == NULL || (*yclosest)->n < yn) {
		return NULL;
	}
	// Found tile, update lookup cache data:
	z->saved_x = *xclosest;
	z->saved_y = *yclosest;
	return (*yclosest)->data;
}

bool
xylist_area_is_covered (struct xylist *l, const struct xylist_req *const req)
{
	bool ret = false;
	struct zoomlevel *z = l->zoom[req->zoom];
	struct xlist *saved_x = z->saved_x;
	struct ytile *saved_y = z->saved_y;
	struct xlist *xclosest = NULL;
	struct ytile *yclosest = NULL;

	// Return true if we can guarantee coverage of the area given by the tile
	// coordinates, and false if we're not positive. Store the cached pointers
	// and restore them at the end, ready for the next walk over the area.
	for (unsigned int xn = req->xmin; xn <= req->xmax; xn++) {
		for (unsigned int yn = req->ymin; yn <= req->ymax; yn++) {
			if (tile_find_cached(z, xn, yn, &xclosest, &yclosest) == NULL) {
				goto out;
			}
		}
	}
	ret = true;

out:	z->saved_x = saved_x;
	z->saved_y = saved_y;
	return ret;
}

static struct xlist *
xlist_insert (struct zoomlevel *z, struct xlist *after, const unsigned int xn)
{
	struct xlist *x;

	if ((x = malloc(sizeof(*x))) == NULL) {
		return NULL;
	}
	x->n = xn;
	x->ys = NULL;
	x->ye = NULL;
	list_init(x);

	// No previous sibling?
	if (after == NULL) {
		// If other elements in list, put this in front:
		if (z->xs != NULL) {
			list_insert_before(z->xs, x);
		}
		// Otherwise this is the first element:
		else {
			z->xe = x;
		}
		z->xs= x;
	}
	else {
		if (z->xe == after) {
			z->xe = x;
		}
		list_insert_after(after, x);
	}
	return x;
}

static struct ytile *
ytile_insert (struct xylist *l, struct zoomlevel *z, struct xlist *x, struct ytile *after, const unsigned int yn, void *const data)
{
	struct ytile *y;

	if ((y = malloc(sizeof(*y))) == NULL) {
		return NULL;
	}
	y->n = yn;
	y->data = data;
	list_init(y);
	l->ntiles++;
	z->ntiles++;

	// No previous sibling?
	if (after == NULL) {
		// If other elements in list, put this in front:
		if (x->ys != NULL) {
			list_insert_before(x->ys, y);
		}
		else {
			x->ye = y;
		}
		x->ys = y;
	}	
	else {
		if (x->ye == after) {
			x->ye = y;
		}
		list_insert_after(after, y);
	}
	return y;
}

static void
cache_purge_x_cols (struct xylist *l, struct zoomlevel *z, const unsigned int xmin, const unsigned int xmax)
{
	struct xlist *xs = z->xs;	// start of x list
	struct xlist *xe = z->xe;	// end of x list

	// xmin and xmax denote the extents of the viewport that we want to preserve.

	while (z->xs)
	{
		// Keep a pointer for the start and end of the list;
		// we will destroy the node that is the furthest from us:
		unsigned int sdist = xmin - xs->n;
		unsigned int edist = xe->n - xmax;

		// Both start and end cols too close to target?
		if (sdist == 0 && edist == 0) {
			break;
		}
		// If distance to start is largest, delete that one:
		if (sdist >= edist) {
			struct xlist *xt = list_next(xs);
			list_detach(z->xs, z->xe, xs);
			xlist_destroy(l, z, &xs);
			if ((xs = xt) == NULL) {
				break;
			}
		}
		// Else delete the end tile:
		else {
			struct xlist *xt = list_prev(xe);
			list_detach(z->xs, z->xe, xe);
			xlist_destroy(l, z, &xe);
			if ((xe = xt) == NULL) {
				break;
			}
		}
		if (z->xs == NULL) {
			break;
		}
		if (l->ntiles <= l->purge_to) {
			break;
		}
	}
}

static void
cache_purge_y_rows (struct xylist *l, struct zoomlevel *z, const unsigned int ymin, const unsigned int ymax)
{
	struct xlist *x = z->xs;

	// See cache_purge_x_cols for explanation.
	// Basically we delete the y tiles which are furthest from current.

	while (x)
	{
		struct xlist *xt = list_next(x);
		struct ytile *ys = x->ys;
		struct ytile *ye = x->ye;

		while (x->ys)
		{
			unsigned int sdist = ymin - ys->n;
			unsigned int edist = ye->n - ymax;

			if (sdist == 0 && edist == 0) {
				break;
			}
			if (sdist >= edist) {
				struct ytile *yt = list_next(ys);
				list_detach(x->ys, x->ye, ys);
				ytile_destroy(l, z, &ys, x->n);
				if ((ys = yt) == NULL) {
					break;
				}
			}
			else {
				struct ytile *yt = list_prev(ye);
				list_detach(x->ys, x->ye, ye);
				ytile_destroy(l, z, &ye, x->n);
				if ((ye = yt) == NULL) {
					break;
				}
			}
			if (l->ntiles <= l->purge_to) {
				break;
			}
		}
		if (x->ys == NULL) {
			list_detach(z->xs, z->xe, x);
			xlist_destroy(l, z, &x);
			break;
		}
		x = xt;
	}
}

static void
cache_purge (struct xylist *l, struct xylist_req *req)
{
	unsigned int z;

	// Purge items from the cache till only l->purge_to tiles left.
	// Use a heuristic method to remove "unimportant" tiles first.

	// First, they came for the far higher zoom levels:
	for (z = l->zoom_max; z > req->zoom + 2; z--) {
		cache_purge_zoomlevel(l, l->zoom[z]);
		if (l->ntiles <= l->purge_to) {
			return;
		}
	}
	// Then, they came for the far lower zoom levels:
	if (req->zoom > l->zoom_min + 2) {
		for (z = l->zoom_min; z < req->zoom - 2; z++) {
			cache_purge_zoomlevel(l, l->zoom[z]);
			if (l->ntiles <= l->purge_to) {
				return;
			}
		}
	}
	// Then, they came for the directly higher zoom levels:
	for (z = l->zoom_max; z > req->zoom; z--) {
		cache_purge_zoomlevel(l, l->zoom[z]);
		if (l->ntiles <= l->purge_to) {
			return;
		}
	}
	// Then, they came for the directly lower zoom levels,
	// but they left the viewport area alone for now:
	for (z = l->zoom_min; z < req->zoom; z++) {
		unsigned int zoomdiff = req->zoom - z;

		cache_purge_x_cols(l, l->zoom[z], req->xmin >> zoomdiff, req->xmax >> zoomdiff);
		if (l->ntiles <= l->purge_to) {
			return;
		}
		cache_purge_y_rows(l, l->zoom[z], req->ymin >> zoomdiff, req->ymax >> zoomdiff);
		if (l->ntiles <= l->purge_to) {
			return;
		}
	}
	// Then, they got the start and end of the x columns,
	// and deleted the furthest columns:
	cache_purge_x_cols(l, l->zoom[req->zoom], req->xmin, req->xmax);
	if (l->ntiles <= l->purge_to) {
		return;
	}
	// Then they went for the y columns:
	cache_purge_y_rows(l, l->zoom[req->zoom], req->ymin, req->ymax);
	if (l->ntiles <= l->purge_to) {
		return;
	}
	// Then they took out the lower zoomlevels entirely:
	for (z = l->zoom_min; z < req->zoom; z++) {
		cache_purge_zoomlevel(l, l->zoom[z]);
	}
	// Then they purged all but the center tile in the current level:
	cache_purge_x_cols(l, l->zoom[req->zoom], (req->xmax - req->xmin) / 2, (req->xmax - req->xmin) / 2);
	if (l->ntiles <= l->purge_to) {
		return;
	}
	cache_purge_y_rows(l, l->zoom[req->zoom], (req->ymax - req->ymin) / 2, (req->ymax - req->ymin) / 2);
	if (l->ntiles <= l->purge_to) {
		return;
	}
	// Then, finally, they got the center tile too:
	cache_purge_zoomlevel(l, l->zoom[req->zoom]);

	// And then there was nothing more to come for...
}

bool
xylist_insert_tile (struct xylist *l, struct xylist_req *req, void *const data)
{
	void *current;
	struct xlist *xclosest = NULL;
	struct ytile *yclosest = NULL;
	struct xlist *xmatch;
	struct ytile *ymatch;
	struct zoomlevel *z = l->zoom[req->zoom];

	// Lets the list owner insert a tile into the list at any time.
	// This can be used by nonblocking tile fetchers. The owner asks the xylist for a tile,
	// the xylist calls the procure function, the procure function immediately returns NULL
	// and shoots off a thread that gets the tile asynchronously in the background.
	// That thread then uses this function to add the tile to the list when it's done.
	// A true return value means "tile successfully inserted", false is "insert failed".

	if (data == NULL || req->zoom < l->zoom_min || req->zoom > l->zoom_max) {
		return false;
	}
	// Must flush cache?
	if (l->ntiles >= l->tiles_max) {
		cache_purge(l, req);
		if (l->ntiles > l->purge_to) {
			return false;
		}
	}
	// Find the closest tile; if exact match, delete existing data:
	if ((current = tile_find_cached(z, req->xn, req->yn, &xclosest, &yclosest)) != NULL) {
		l->tile_destroy(current);
		yclosest->data = data;
		return true;
	}
	// Otherwise, create a new tile at requested position. First create x col if we have to:
	if (xclosest == NULL || xclosest->n != req->xn) {
		if ((xmatch = xlist_insert(z, xclosest, req->xn)) == NULL) {
			return false;
		}
	}
	else {
		xmatch = xclosest;
	}
	// We're on the right x col, now create the y tile:
	if ((ymatch = ytile_insert(l, z, xmatch, yclosest, req->yn, data)) == NULL) {
		return false;
	}
	return true;
}

bool
xylist_delete_tile (struct xylist *l, struct xylist_req *req)
{
	struct xlist *xclosest = NULL;
	struct ytile *yclosest = NULL;
	struct zoomlevel *z = l->zoom[req->zoom];

	if (req->zoom < l->zoom_min || req->zoom > l->zoom_max) {
		return false;
	}
	// Find the tile:
	if (tile_find_cached(z, req->xn, req->yn, &xclosest, &yclosest) == NULL
	 || xclosest == NULL || xclosest->n != req->xn
	 || yclosest == NULL || yclosest->n != req->yn) {
		return false;
	}
	list_detach(xclosest->ys, xclosest->ye, yclosest);
	ytile_destroy(l, z, &yclosest, req->xn);
	if (xclosest->ys == NULL) {
		list_detach(z->xs, z->xe, xclosest);
		xlist_destroy(l, z, &xclosest);
	}
	return true;
}

void *
xylist_request (struct xylist *l, struct xylist_req *req)
{
	void *data;
	struct xlist *xclosest = NULL;
	struct ytile *yclosest = NULL;
	struct xlist *xmatch;
	struct ytile *ymatch;
	struct zoomlevel *z;

	// Is the request invalid or out of bounds?
	// TODO: for efficiency, perhaps remove this check:
	if (l == NULL || req->zoom < l->zoom_min || req->zoom > l->zoom_max) {
		return NULL;
	}
	z = l->zoom[req->zoom];
	// Can we find the tile in the cache?
start:	if ((data = tile_find_cached(z, req->xn, req->yn, &xclosest, &yclosest)) != NULL) {
		return data;
	}
	if (req->search_depth == 0) {
		return NULL;
	}
	// Can't fetch the tile from the cache; try to make a new tile.
	// Purge items from the cache if it's getting too full:
	if (l->ntiles >= l->tiles_max) {
		cache_purge(l, req);
		if (l->ntiles > l->purge_to) {
			return NULL;
		}
		// At this point we cannot trust xclosest and yclosest any more; regenerate:
		xclosest = NULL;
		yclosest = NULL;
		goto start;
	}
	// issue a callback to procure the tile data; if this fails, we're sunk:
	req->search_depth--;
	if (l->tile_procure == NULL || (data = l->tile_procure(req)) == NULL) {
		return NULL;
	}
	// New data; hang it in to the xylist after the closest x and y members:
	// Do we already have the right x col?
	if (xclosest == NULL || xclosest->n != req->xn) {
		// Nope, try to create it in place:
		if ((xmatch = xlist_insert(z, xclosest, req->xn)) == NULL) {
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
	if ((ymatch = ytile_insert(l, z, xmatch, yclosest, req->yn, data)) == NULL) {
		// Again, this should not happen; trash our freshly procured data:
		l->tile_destroy(data);
		return NULL;
	}
	// Update lookup cache values:
	z->saved_x = xmatch;
	z->saved_y = ymatch;
	return data;
}
