#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <png.h>

#include "png.h"

// Structure for memory read I/O:
struct io {
	const char	*buf;
	const char	*cur;
	size_t		 len;
};

// Our own I/O functions:
static void
read_fn (png_structp png_ptr, png_bytep data, png_size_t length)
{
	struct io *io = png_get_io_ptr(png_ptr);

	// Return if asking for more data than available:
	if (length > io->len - (io->cur - io->buf)) {
		png_error(png_ptr, NULL);
		return;
	}

	// Copy data and adjust pointers:
	memcpy(data, io->cur, length);
	io->cur += length;
}

static void
error_fn (png_structp png_ptr, png_const_charp msg)
{
	(void)msg;

	// According to the libpng docs, this function should not return
	// control, but longjmp back to the caller:
	longjmp(png_jmpbuf(png_ptr), 1);
}

static void
warning_fn (png_structp png_ptr, png_const_charp msg)
{
	(void)png_ptr;
	(void)msg;

	// According to the docs, a warning should simply return, not longjmp.
}

static bool
is_png (const char *buf, size_t len)
{
	return !png_sig_cmp((png_bytep)buf, 0, (len > 8) ? 8 : len);
}

bool
png_load (const char *data, size_t len, unsigned int *height, unsigned int *width, char **rawbits)
{
	png_structp png_ptr;
	png_infop info_ptr;
	bool err = false;
	char channels;

	// Initialize caller-supplied pointer:
	*rawbits = NULL;

	// Data must look like a PNG file:
	if (!is_png(data, len))
		return false;

	// Create read struct:
	png_ptr = png_create_read_struct
		( PNG_LIBPNG_VER_STRING
		, NULL
		, error_fn
		, warning_fn
		) ;

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
	png_set_read_fn(png_ptr, &io, read_fn);

	// Return here on errors:
	if (setjmp(png_jmpbuf(png_ptr))) {
		err = true;
		goto exit;
	}

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
	channels = png_get_channels(png_ptr, info_ptr);

	// Allocate the rawbits image data:
	if ((*rawbits = malloc(*width * *height * channels)) == NULL) {
		err = true;
		goto exit;
	}

	// Allocate array of row pointers, read actual image data:
	// TODO: check if it's wise to allocate this on the stack:
	{
		png_byte *b = *rawbits;
		size_t row_stride = *width * channels;
		png_bytep row_pointers[*height];

		for (unsigned int i = 0; i < *height; i++) {
			row_pointers[i] = b;
			b += row_stride;
		}
		png_read_image(png_ptr, row_pointers);
	}
	png_read_end(png_ptr, NULL);

	// Cleanup:
exit:	png_destroy_read_struct(&png_ptr, &info_ptr, NULL);

	if (!err)
		return true;

	free(*rawbits);
	*rawbits = NULL;
	return false;
}
