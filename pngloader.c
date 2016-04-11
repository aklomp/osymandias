#include <stdbool.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <png.h>

#include "quadtree.h"
#include "diskcache.h"
#include "png.h"
#include "pngloader.h"

#define HEAP_SIZE	100000
#define TILESIZE	256	// rgb pixels per side

#ifndef __BIGGEST_ALIGNMENT__	// FIXME: hack to compile with clang
#define __BIGGEST_ALIGNMENT__	16
#endif

// Structure for memory read I/O:
struct io {
	size_t      len;
	const char *buf;
	const char *cur;
};

static __thread char *heap = NULL;
static __thread char *heap_head = NULL;
static __thread bool initialized = false;
static __thread volatile bool cancel_flag = false;

static bool
load_png_file (int fd, unsigned int *height, unsigned int *width, char **rawbits)
{
	bool ret = false;
	char *data;
	struct stat stat;
	size_t len;
	ssize_t nread;

	// Get file size:
	if (fstat(fd, &stat))
		return false;

	// Allocate buffer:
	if ((data = malloc(len = stat.st_size)) == NULL)
		return false;

	// Read in entire file, check carefully for cancellation:
	for (size_t total = 0; total < len; total += nread) {
		nread = read(fd, data + total, len - total);
		if (cancel_flag || nread <= 0) {
			free(data);
			return false;
		}
	}

	// Pass to png loader:
	ret = png_load(data, len, height, width, rawbits);

	// Clean up buffer:
	free(data);
	return ret;
}

void
pngloader_main (void *data)
{
	int fd = -1;
	struct pngloader *p = data;
	unsigned int width;
	unsigned int height;
	char *rawbits = NULL;

	if (!initialized) {
		free(p);
		return;
	}
	// Reset heap pointer:
	heap_head = heap;

	cancel_flag = false;

	if ((p->filename = tile_filename(p->req.zoom, p->req.x, p->req.y)) == NULL || cancel_flag)
		goto exit;

	// Get a file descriptor to the file. We just want a file descriptor to
	// associate with this thread so that we know what to clean up, and not
	// wait for the disk to actually deliver, so we issue a nonblocking call:
	if ((fd = open(p->filename, O_RDONLY | O_NONBLOCK)) < 0 || cancel_flag)
		goto exit;

	// Now that we have the fd, make it properly blocking:
	if (fcntl(fd, F_SETFL, fcntl(fd, F_GETFL) & ~O_NONBLOCK) < 0 || cancel_flag)
		goto exit;

	if (!load_png_file(fd, &height, &width, &rawbits) || cancel_flag)
		goto exit;

	if (fd >= 0) {
		close(fd);
		fd = -1;
	}
	if (height != TILESIZE || width != TILESIZE) {
		free(rawbits);
		rawbits = NULL;
		goto exit;
	}
	// Got tile, run callback:
	p->on_completed(p, rawbits);

exit:	if (fd >= 0)
		close(fd);

	free(p->filename);
	free(p);
}

void
pngloader_on_cancel (void)
{
	cancel_flag = true;
}

void
pngloader_on_init (void)
{
	// Allocate a block of heap memory:
	if ((heap = heap_head = malloc(HEAP_SIZE)) != NULL)
		initialized = true;
}

void
pngloader_on_exit (void)
{
	free(heap);
}
