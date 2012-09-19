#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <pthread.h>
#include <png.h>

#include "xylist.h"
#include "pngloader.h"

static __thread int fd = -1;
static __thread FILE *fp = NULL;
static __thread void *rawbits = NULL;
static __thread png_structp png_ptr = NULL;
static __thread png_infop info_ptr = NULL;

static bool
viking_filename (char *buf, size_t buflen, unsigned int zoom, int tile_x, int tile_y)
{
	size_t len;
	char *home;

	// FIXME: horrible copypaste of code from viewport.c:
	unsigned int world_size = ((unsigned int)1) << (zoom + 8);

	if ((home = getenv("HOME")) == NULL) {
		return false;
	}
	if ((len = snprintf(buf, buflen, "%s/.viking-maps/t13s%uz0/%u/%u", home, 17 - zoom, tile_x, world_size / 256 - 1 - tile_y)) == buflen) {
		return false;
	}
	buf[len] = '\0';
	return true;
}

static void
pngloader_cancel (void *data)
{
	struct pngloader *p = data;

	if (fp != NULL) {
		fclose(fp);
	}
	else if (fd >= 0) {
		close(fd);
	}
	png_destroy_read_struct(&png_ptr, &info_ptr, NULL);
	free(rawbits);
	free(p);
}

static bool
load_png_file (png_structp *ppng_ptr, png_infop *pinfo_ptr, unsigned int *height, unsigned int *width, void **rawbits)
{
	png_structp png_ptr = *ppng_ptr;
	png_infop info_ptr = *pinfo_ptr;
	bool ret = false;

	*rawbits = NULL;

	// TODO: we don't handle errors; what happens when they occur?
	if ((png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL)) == NULL) {
		goto exit_0;
	}
	if ((info_ptr = png_create_info_struct(png_ptr)) == NULL) {
		goto exit_1;
	}
	if (setjmp(png_jmpbuf(png_ptr))) {
		goto exit_1;
	}
	// Check that the file is a PNG; if so, init the i/o functions:
	{
		char buf[8];

		if (fread(buf, 1, sizeof(buf), fp) != sizeof(buf)) {
			goto exit_1;
		}
		if (png_sig_cmp((png_bytep)buf, 0, sizeof(buf))) {
			goto exit_1;
		}
		png_init_io(png_ptr, fp);
		png_set_sig_bytes(png_ptr, sizeof(buf));
	}
	// Sane limits on image:
	png_set_user_limits(png_ptr, 500, 500);

	// Read PNG up to image data:
	png_read_info(png_ptr, info_ptr);

	// Transform input into 8-bit RGB:
	{
		png_byte color_type = png_get_color_type(png_ptr, info_ptr);
		png_byte bit_depth = png_get_bit_depth(png_ptr, info_ptr);

		// Convert paletted images to RGB:
		if (color_type == PNG_COLOR_TYPE_PALETTE) {
			png_set_palette_to_rgb(png_ptr);
		}
		// Upsample low-bit grayscale images to 8-bit:
		if (color_type == PNG_COLOR_TYPE_GRAY && bit_depth < 8) {
			png_set_expand_gray_1_2_4_to_8(png_ptr);
		}
		// Upsample low-bit images to 8-bit:
		if (bit_depth < 8) {
			png_set_packing(png_ptr);
		}
		// Discard alpha channel:
		if (color_type & PNG_COLOR_MASK_ALPHA) {
			png_set_strip_alpha(png_ptr);
		}
		// Upsample grayscale to RGB:
		if (color_type == PNG_COLOR_TYPE_GRAY
		 || color_type == PNG_COLOR_TYPE_GRAY_ALPHA) {
			png_set_gray_to_rgb(png_ptr);
		}
		png_read_update_info(png_ptr, info_ptr);
	}
	*width = png_get_image_width(png_ptr, info_ptr);
	*height = png_get_image_height(png_ptr, info_ptr);

	// Allocate the rawbits image data:
	if ((*rawbits = malloc(*width * *height * 3)) == NULL) {
		goto exit_1;
	}
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
	ret = true;

exit_1:	png_destroy_read_struct(&png_ptr, &info_ptr, NULL);
	if (ret == false) {
		free(*rawbits);
		*rawbits = NULL;
	}
exit_0:	return ret;
}

void *
pngloader_main (void *data)
{
	bool found = 0;
	char filename[100];
	struct pngloader *p;
	unsigned int width;
	unsigned int height;

	pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, NULL);
	pthread_cleanup_push(pngloader_cancel, data);

	p = data;

	if (!viking_filename(filename, sizeof(filename), p->req.zoom, p->req.xn, p->req.yn)) {
		goto exit;
	}
	// Get a file descriptor to the file. We just want a file descriptor to
	// associate with this thread so that we know what to clean up, and not
	// wait for the disk to actually deliver, so we issue a nonblocking call:
	if ((fd = open(filename, O_RDONLY | O_NONBLOCK)) < 0) {
		goto exit;
	}
	// Now that we have the fd, make it properly blocking:
	fcntl(fd, F_SETFL, fcntl(fd, F_GETFL) & ~O_NONBLOCK);

	// From here on, live dangerously:
/****/	pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);

	// Now open an fp from the descriptor and load the image:
	// (this is the blocking part of the thread, we hope):
	if ((fp = fdopen(fd, "rb")) == NULL) {
		goto exit;
	}
	bool ret = load_png_file(&png_ptr, &info_ptr, &height, &width, &rawbits);
/****/	pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, NULL);
	if (!ret) {
		goto exit;
	}
	if (fp != NULL) {
		fclose(fp);
		fp = NULL;
		fd = -1;
	}
	if (height != 256 || width != 256) {
		free(rawbits);
		rawbits = NULL;
		goto exit;
	}
	// Got tile, insert into bitmaps:
	ret = false;
	found = true;
	// pointer to bitmaps can become NULL if the parent process is
	// suddenly shutting down on us:
	pthread_mutex_lock(p->bitmaps_mutex);
	if (p->bitmaps != NULL) {
		ret = xylist_insert_tile(*(p->bitmaps), &p->req, rawbits);
	}
	pthread_mutex_unlock(p->bitmaps_mutex);
	if (!ret) {
		free(rawbits);
		rawbits = NULL;
	}
	else if (p->completed_callback != NULL) {
		p->completed_callback();
	}

exit:	pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, NULL);
	if (fp != NULL) {
		fclose(fp);
		fp = NULL;
		fd = -1;
	}
	else if (fd >= 0) {
		close(fd);
		fd = -1;
	}
	png_destroy_read_struct(&png_ptr, &info_ptr, NULL);

	// Really rudimentary rate-limiting to occupy this (zoom,xn,yn) slot a
	// little longer, ensurng that unavailable tiles aren't re-probed at
	// every screen refresh; if the slot is needed, this thread will be
	// reaped here:
	if (!found) {
/****/		pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
		sleep(5);
/****/		pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, NULL);
	}
	// From here on we are not cancelable; get rid of cleanup:
	pthread_cleanup_pop(0);
	// We delete ourselves from the threads xylist. This will free up a
	// tile. but not cancel us, since we check for suicide:
	pthread_mutex_lock(p->threadlist_mutex);
	if (p->threadlist != NULL) {
		xylist_delete_tile(*(p->threadlist), &p->req);
	}
	pthread_mutex_unlock(p->threadlist_mutex);
	pthread_mutex_lock(p->running_mutex);
	p->running = 0;
	pthread_mutex_unlock(p->running_mutex);
	free(p);
	pthread_exit(NULL);
}
