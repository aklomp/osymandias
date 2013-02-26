#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>

#include "world.h"

// Return a malloc()'ed string containing the filename,
// to be freed by the caller with free(), or NULL on error.

char *
tile_filename (unsigned int zoom, int tile_x, int tile_y)
{
	// Initial guess for string length:
	size_t buflen = 50;
	int retlen;
	char *name;
	char *temp;
	char *home;

	if ((home = getenv("HOME")) == NULL) {
		return NULL;
	}
	if ((name = malloc(buflen)) == NULL) {
		return NULL;
	}
	for (;;) {
		// FIXME: actual customizable pattern here:
		retlen = snprintf(name, buflen, "%s/.viking-maps/t13s%uz0/%u/%u", home, 17 - zoom, tile_x, world_get_size() / 256 - 1 - tile_y);
		if (retlen > -1 && (size_t)retlen < buflen) {
			return name;
		}
		// Resize the buffer; if retlen positive, it is the
		// exact size we need; else double the current estimate:
		buflen = (retlen > -1) ? (size_t)retlen + 1 : buflen * 2;
		if ((temp = realloc(name, buflen)) == NULL) {
			free(name);
			return NULL;
		}
		name = temp;
	}
}
