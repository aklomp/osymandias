#include <stdbool.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#include "diskcache.h"
#include "png.h"
#include "pngloader.h"

#define TILESIZE	256	// rgb pixels per side

// Return the contents of a file.
static void *
read_file (const int fd, size_t *len)
{
	char *buf = NULL;
	struct stat stat;
	ssize_t nread;

	// Get file size:
	if (fstat(fd, &stat))
		return NULL;

	// Allocate buffer:
	if ((buf = malloc(*len = stat.st_size)) == NULL)
		return NULL;

	// Read in entire file:
	for (size_t total = 0; total < *len; total += (size_t) nread) {
		nread = read(fd, buf + total, *len - total);
		if (nread <= 0) {
			free(buf);
			return NULL;
		}
	}

	return buf;
}

// Return raw PNG data for the request.
static void *
read_pngdata (const struct cache_node *req, size_t *len)
{
	int fd;
	char *buf = NULL;

	if ((fd = diskcache_open(req->zoom, req->x, req->y)) < 0)
		return NULL;

	// Now that we have the fd, make it properly blocking:
	if (fcntl(fd, F_SETFL, fcntl(fd, F_GETFL) & ~O_NONBLOCK) == 0)
		buf = read_file(fd, len);

	close(fd);
	return buf;
}

void *
pngloader_main (const struct cache_node *req)
{
	struct png_in  in = { .name = "cache request" };
	struct png_out out;

	if ((in.buf = read_pngdata(req, &in.len)) == NULL)
		return NULL;

	if (png_load(&in, &out) == false) {
		free((void *) in.buf);
		return NULL;
	}

	free((void *) in.buf);
	if (out.height == TILESIZE && out.width == TILESIZE)
		return out.buf;

	free(out.buf);
	return NULL;
}
