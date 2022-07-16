#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <png.h>

#include "png.h"

// Structure for memory read I/O.
struct io {
	const uint8_t *buf;
	const uint8_t *cur;
	size_t         len;
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
	const struct png_in *in = png_get_error_ptr(png_ptr);

	fprintf(stderr, "png: error: %s: %s\n", in->name, msg);

	// According to the libpng docs, this function should not return
	// control, but longjmp back to the caller:
	longjmp(png_jmpbuf(png_ptr), 1);
}

static void
warning_fn (png_structp png_ptr, png_const_charp msg)
{
	const struct png_in *in = png_get_error_ptr(png_ptr);

	fprintf(stderr, "png: warning: %s: %s\n", in->name, msg);

	// According to the docs, a warning should simply return, not longjmp.
}

static bool
is_png (const struct png_in *in)
{
	return png_sig_cmp(in->buf, 0, (in->len > 8) ? 8 : in->len) == 0;
}

bool
png_load (const struct png_in *in, struct png_out *out)
{
	png_structp png_ptr;
	png_infop info_ptr;
	bool err = false;

	// Reset the caller-supplied output pointer.
	out->buf = NULL;

	// The user's data must look like a PNG file.
	if (!is_png(in))
		return false;

	// Create the read structure.
	png_ptr = png_create_read_struct(
		PNG_LIBPNG_VER_STRING,
		(void *) in,
		error_fn,
		warning_fn
	);

	if (png_ptr == NULL)
		return false;

	// Create the info structure.
	if ((info_ptr = png_create_info_struct(png_ptr)) == NULL) {
		png_destroy_read_struct(&png_ptr, &info_ptr, NULL);
		return false;
	}

	// Set up the read I/O structure.
	struct io io = {
		.len = in->len,
		.buf = in->buf,
		.cur = in->buf,
	};

	// Register a custom read function.
	png_set_read_fn(png_ptr, &io, read_fn);

	// Return here on errors.
	if (setjmp(png_jmpbuf(png_ptr))) {
		err = true;
		goto exit;
	}

	// Read the PNG up to the image data.
	png_read_info(png_ptr, info_ptr);

	// Transform the input to 8-bit RGB.
	png_byte color_type = png_get_color_type (png_ptr, info_ptr);
	png_byte bit_depth  = png_get_bit_depth  (png_ptr, info_ptr);

	// Convert paletted images to RGB.
	if (color_type == PNG_COLOR_TYPE_PALETTE)
		png_set_palette_to_rgb(png_ptr);

	// Upsample low-bit grayscale images to 8-bit.
	if (color_type == PNG_COLOR_TYPE_GRAY && bit_depth < 8)
		png_set_expand_gray_1_2_4_to_8(png_ptr);

	// Upsample low-bit images to 8-bit.
	if (bit_depth < 8)
		png_set_packing(png_ptr);

	// Discard the alpha channel.
	if (color_type & PNG_COLOR_MASK_ALPHA)
		png_set_strip_alpha(png_ptr);

	// Upsample grayscale to RGB.
	if (color_type == PNG_COLOR_TYPE_GRAY
	 || color_type == PNG_COLOR_TYPE_GRAY_ALPHA)
		png_set_gray_to_rgb(png_ptr);

	// Update the image info and populate the output structure.
	png_read_update_info(png_ptr, info_ptr);
	out->width    = png_get_image_width  (png_ptr, info_ptr);
	out->height   = png_get_image_height (png_ptr, info_ptr);
	out->channels = png_get_channels     (png_ptr, info_ptr);

	// Allocate the image data.
	if ((out->buf = malloc(out->width * out->height * out->channels)) == NULL) {
		err = true;
		goto exit;
	}

	// Allocate array of row pointers, read actual image data:
	// TODO: check if it's wise to allocate this on the stack:
	{
		png_byte *b = out->buf;
		size_t row_stride = out->width * out->channels;
		png_bytep row_pointers[out->height];

		for (uint16_t i = 0; i < out->height; i++) {
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

	free(out->buf);
	out->buf = NULL;
	return false;
}
