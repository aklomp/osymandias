#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "pngloader.h"

#define N_BITMAPS	1000	// Keep this many raw tiles in the cache

struct bitmap {
	void *rawbits;
	unsigned int x;
	unsigned int y;
	unsigned int zoom;
};

static struct bitmap *bitmap_create (const unsigned int zoom, const unsigned int x, const unsigned int y);

struct bitmap **bitmaps = NULL;

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

static struct bitmap *
bitmap_find_cached (const unsigned int zoom, const unsigned int x, const unsigned int y)
{
	for (struct bitmap **b = bitmaps; b < bitmaps + N_BITMAPS; b++) {
		if (*b == NULL) {
			continue;
		}
		if ((*b)->zoom == zoom && (*b)->x == x && (*b)->y == y) {
			return *b;
		}
	}
	return NULL;
}

void *
bitmap_get (const unsigned int zoom, const unsigned int x, const unsigned int y)
{
	struct bitmap **b;
	struct bitmap *s;

	// Do we already have the tile?
	if ((s = bitmap_find_cached(zoom, x, y)) != NULL) {
		return s->rawbits;
	}
	// Get next free tile slot:
	for (b = bitmaps; b < bitmaps + N_BITMAPS; b++) {
		if (*b == NULL) {
			break;
		}
	}
	// No free space? Delete a tile:
	if (b == bitmaps + N_BITMAPS) {
		return NULL;
	}
	// Create the tile:
	if ((s = bitmap_create(zoom, x, y)) == NULL) {
		return NULL;
	}
	// Store pointer to tile in array:
	*b = s;
	return s->rawbits;
}

static struct bitmap *
bitmap_create (const unsigned int zoom, const unsigned int x, const unsigned int y)
{
	char *filename;
	void *rawbits;
	struct bitmap *b;
	unsigned int width;
	unsigned int height;

	if ((filename = viking_filename(zoom, x, y)) == NULL) {
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
	if ((b = malloc(sizeof(*b))) == NULL) {
		free(rawbits);
		return NULL;
	}
	b->rawbits = rawbits;
	b->zoom = zoom;
	b->x = x;
	b->y = y;
	return b;
}

static void
bitmap_destroy (struct bitmap *b)
{
	if (b) {
		free(b->rawbits);
		free(b);
	}
}

bool
bitmap_mgr_init (void)
{
	// Allocate array of N_RAWTILES struct pointers:
	if ((bitmaps = malloc(sizeof(*bitmaps) * N_BITMAPS)) == NULL) {
		return false;
	}
	memset(bitmaps, 0, sizeof(*bitmaps) * N_BITMAPS);
	return true;
}

void
bitmap_mgr_destroy (void)
{
	if (bitmaps) {
		struct bitmap **b;
		for (b = bitmaps; b < bitmaps + N_BITMAPS; b++) {
			bitmap_destroy(*b);
		}
		free(bitmaps);
	}
}
