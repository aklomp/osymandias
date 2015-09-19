#include <stdbool.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <png.h>

#include "quadtree.h"
#include "diskcache.h"
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

static __thread void *rawbits = NULL;
static __thread char *heap = NULL;
static __thread char *heap_head = NULL;
static __thread int init_ok = 0;
static __thread volatile bool cancel_flag = false;

// Our private malloc/free functions that we give to libpng.
// We just allocate everything off a heap sequentially and never
// bother to free. At completion or cancellation, we release the
// entire pool. This should cut down on memory leaks.

static png_voidp
malloc_fn (png_structp png_ptr, png_size_t size)
{
	(void)png_ptr;

	char *ret = heap_head;

	// Increment size to next alignment boundary:
	if (size & (__BIGGEST_ALIGNMENT__ - 1)) {
		size &= ~(__BIGGEST_ALIGNMENT__ - 1);
		size += __BIGGEST_ALIGNMENT__;
	}
	// Heap should be big enough; check just in case:
	if (((heap_head + size) - heap) > HEAP_SIZE)
		return NULL;

	heap_head += size;
	return (png_voidp)ret;
}

static void
free_fn (png_structp png_ptr, png_voidp ptr)
{
	(void)png_ptr;
	(void)ptr;

	// We don't free anything.
}

// Our own I/O functions:
static void
user_read_data (png_structp png_ptr, png_bytep data, png_size_t length)
{
	struct io *io = png_get_io_ptr(png_ptr);

	// Canceled or asking for more data than available:
	if (cancel_flag || length > io->len - (io->cur - io->buf)) {
		png_error(png_ptr, NULL);
		return;
	}

	// Copy data and adjust pointers:
	memcpy(data, io->cur, length);
	io->cur += length;
}

static void
user_error_fn (png_structp png_ptr, png_const_charp msg)
{
	(void)png_ptr;
	(void)msg;

	// According to the libpng docs, this function should not return control,
	// but longjmp back to the caller:
	cancel_flag = true;
	longjmp(png_jmpbuf(png_ptr), 1);
}

static void
user_warning_fn (png_structp png_ptr, png_const_charp msg)
{
	(void)png_ptr;
	(void)msg;

	// According to the docs, the warning should simply return, not longjmp.
	cancel_flag = true;
}

void
pngloader_on_init (void)
{
	// Allocate a block of heap memory:
	if ((heap = heap_head = malloc(HEAP_SIZE)) == NULL) {
		return;
	}
	init_ok = 1;
}

static bool
is_png (const char *buf, size_t len)
{
	return !png_sig_cmp((png_bytep)buf, 0, (len > 8) ? 8 : len);
}

static bool
load_png (const char *data, size_t len, unsigned int *height, unsigned int *width, void **rawbits)
{
	bool ret = false;
	png_structp png_ptr;
	png_infop info_ptr;

	// Must look like a PNG:
	if (!is_png(data, len))
		return false;

	// Create read struct:
	png_ptr = png_create_read_struct_2(
		PNG_LIBPNG_VER_STRING,
		NULL,
		user_error_fn,
		user_warning_fn,
		NULL,
		malloc_fn,
		free_fn
	);

	if (png_ptr == NULL)
		return false;

	// Create info struct:
	if ((info_ptr = png_create_info_struct(png_ptr)) == NULL) {
		png_destroy_read_struct(&png_ptr, &info_ptr, NULL);
		return false;
	}

	// Set up read I/O structure:
	struct io io = {
		.len = len,
		.buf = data,
		.cur = data,
	};

	// Instantiate our own read function:
	png_set_read_fn(png_ptr, &io, user_read_data);

	// Return here on errors:
	if (setjmp(png_jmpbuf(png_ptr)))
		goto exit;

	// Sane limits on image:
	png_set_user_limits(png_ptr, 500, 500);

	// Read PNG up to image data:
	png_read_info(png_ptr, info_ptr);

	// Transform input into 8-bit RGB:
	png_byte color_type = png_get_color_type (png_ptr, info_ptr);
	png_byte bit_depth  = png_get_bit_depth  (png_ptr, info_ptr);

	// Convert paletted images to RGB:
	if (color_type == PNG_COLOR_TYPE_PALETTE)
		png_set_palette_to_rgb(png_ptr);

	// Upsample low-bit grayscale images to 8-bit:
	if (color_type == PNG_COLOR_TYPE_GRAY && bit_depth < 8)
		png_set_expand_gray_1_2_4_to_8(png_ptr);

	// Upsample low-bit images to 8-bit:
	if (bit_depth < 8)
		png_set_packing(png_ptr);

	// Discard alpha channel:
	if (color_type & PNG_COLOR_MASK_ALPHA)
		png_set_strip_alpha(png_ptr);

	// Upsample grayscale to RGB:
	if (color_type == PNG_COLOR_TYPE_GRAY
	 || color_type == PNG_COLOR_TYPE_GRAY_ALPHA)
		png_set_gray_to_rgb(png_ptr);

	png_read_update_info(png_ptr, info_ptr);

	*width  = png_get_image_width  (png_ptr, info_ptr);
	*height = png_get_image_height (png_ptr, info_ptr);

	// Allocate the rawbits image data:
	if ((*rawbits = malloc(*width * *height * 3)) == NULL)
		goto exit;

	// Allocate array of row pointers, read actual image data:
	// TODO: check if it's wise to allocate this on the stack:
	{
		png_byte *b = *rawbits;
		size_t row_stride = *width * 3;
		png_bytep row_pointers[*height];

		for (unsigned int i = 0; i < *height; i++) {
			row_pointers[i] = b;
			b += row_stride;
		}
		png_read_image(png_ptr, row_pointers);
	}
	png_read_end(png_ptr, NULL);
	ret = !cancel_flag;

exit:	png_destroy_read_struct(&png_ptr, &info_ptr, NULL);

	if (ret)
		return true;

	free(*rawbits);
	*rawbits = NULL;
	return false;
}

static bool
load_png_file (int fd, unsigned int *height, unsigned int *width, void **rawbits)
{
	bool ret = false;
	char *data;
	struct stat stat;
	size_t len;
	ssize_t nread;

	*rawbits = NULL;

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
	ret = load_png(data, len, height, width, rawbits);

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

	if (!init_ok) {
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
	if (!init_ok)
		return;

	cancel_flag = true;
}

void
pngloader_on_exit (void)
{
	if (!init_ok)
		return;

	free(heap);
}
