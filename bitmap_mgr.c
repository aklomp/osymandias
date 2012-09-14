#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "pngloader.h"
#include "xylist.h"

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

static struct xylist *bitmaps = NULL;

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

void *
bitmap_request (struct xylist_req *req)
{
	return xylist_request(bitmaps, req);
}

static void *
rawbits_procure (struct xylist_req *req)
{
	char *filename;
	void *rawbits;
	unsigned int width;
	unsigned int height;

	if ((filename = viking_filename(req->zoom, req->xn, req->yn)) == NULL) {
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
	return rawbits;
}

static void
rawbits_destroy (void *data)
{
	free(data);
}

bool
bitmap_mgr_init (void)
{
	return ((bitmaps = xylist_create(0, ZOOM_MAX, MAX_BITMAPS, &rawbits_procure, &rawbits_destroy)) == NULL) ? 0 : 1;
}

void
bitmap_mgr_destroy (void)
{
	xylist_destroy(&bitmaps);
}
