#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

// Return a malloc()'ed string containing the filename,
// to be freed by the caller with free(), or NULL on error.

static char *
get_filename (unsigned int zoom, int tile_x, int tile_y)
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
		retlen = snprintf(name, buflen, "%s/.viking-maps/t13s%uz0/%u/%u", home, 17 - zoom, tile_x, tile_y);
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

// Add given blob to the disk cache.
bool
diskcache_add (unsigned int zoom, int tile_x, int tile_y, const char *data, size_t size)
{
	int fd;
	char *filename;
	ssize_t nwrite;

	if ((filename = get_filename(zoom, tile_x, tile_y)) == NULL)
		return false;

	if ((fd = open(filename, O_WRONLY | O_CREAT, S_IRUSR | S_IWUSR)) < 0) {
		free(filename);
		return false;
	}

	while (size) {
		if ((nwrite = write(fd, data, size)) < 0) {
			if (errno == EINTR)
				continue;

			close(fd);
			unlink(filename);
			free(filename);
			return false;
		}
		data += nwrite;
		size -= nwrite;
	}

	close(fd);
	free(filename);
	return true;
}

// Remove blob stored at given location.
bool
diskcache_del (unsigned int zoom, int tile_x, int tile_y)
{
	int ret;
	char *filename;

	if ((filename = get_filename(zoom, tile_x, tile_y)) == NULL)
		return false;

	ret = unlink(filename);
	free(filename);
	return (ret == 0);
}

// Open file containing blob, return file handle.
int
diskcache_open (unsigned int zoom, int tile_x, int tile_y)
{
	int fd;
	char *filename;

	if ((filename = get_filename(zoom, tile_x, tile_y)) == NULL)
		return false;

	fd = open(filename, O_RDONLY | O_NONBLOCK);
	free(filename);
	return fd;
}
