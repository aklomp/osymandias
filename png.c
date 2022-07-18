#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <png.h>

#include "png.h"

// Local state structure.
struct state {
	png_structp pngp;
	png_infop   infop;

	// Structure for memory read I/O.
	struct io {
		const uint8_t *buf;
		const uint8_t *cur;
		size_t         len;
	} io;
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

static void
deinit (struct state *state)
{
	png_destroy_read_struct(&state->pngp, &state->infop, NULL);
}

static bool
init (struct state *state, const struct png_in *in, struct png_out *out)
{
	// Reset the caller-supplied output pointer.
	out->buf = NULL;

	// The user's data must look like a PNG file.
	if (!is_png(in))
		return false;

	// Create the read structure.
	state->pngp = png_create_read_struct(
		PNG_LIBPNG_VER_STRING,
		(void *) in,
		error_fn,
		warning_fn
	);

	if (state->pngp == NULL)
		return false;

	// Create the info structure.
	if ((state->infop = png_create_info_struct(state->pngp)) == NULL) {
		deinit(state);
		return false;
	}

	// Set up the read I/O structure.
	state->io = (struct io) {
		.len = in->len,
		.buf = in->buf,
		.cur = in->buf,
	};

	// Register a custom read function.
	png_set_read_fn(state->pngp, &state->io, read_fn);

	return true;
}

static bool
load (struct state *state, struct png_out *out)
{
	// Read the PNG up to the image data.
	png_read_info(state->pngp, state->infop);

	// Transform the input to 8-bit RGB.
	png_byte color_type = png_get_color_type (state->pngp, state->infop);
	png_byte bit_depth  = png_get_bit_depth  (state->pngp, state->infop);

	// Convert paletted images to RGB.
	if (color_type == PNG_COLOR_TYPE_PALETTE)
		png_set_palette_to_rgb(state->pngp);

	// Upsample low-bit grayscale images to 8-bit.
	if (color_type == PNG_COLOR_TYPE_GRAY && bit_depth < 8)
		png_set_expand_gray_1_2_4_to_8(state->pngp);

	// Upsample low-bit images to 8-bit.
	if (bit_depth < 8)
		png_set_packing(state->pngp);

	// Discard the alpha channel.
	if (color_type & PNG_COLOR_MASK_ALPHA)
		png_set_strip_alpha(state->pngp);

	// Upsample grayscale to RGB.
	if (color_type == PNG_COLOR_TYPE_GRAY
	 || color_type == PNG_COLOR_TYPE_GRAY_ALPHA)
		png_set_gray_to_rgb(state->pngp);

	// Update the image info and populate the output structure.
	png_read_update_info(state->pngp, state->infop);
	out->width    = png_get_image_width  (state->pngp, state->infop);
	out->height   = png_get_image_height (state->pngp, state->infop);
	out->channels = png_get_channels     (state->pngp, state->infop);

	// Define a pointer to an image array with the dimensions and depth of
	// the target image. This lets us offload all offset calculations to
	// the compiler by (ab)using sizeof() and array indexing syntax.
	uint8_t (*img)[out->height][out->width][out->channels];

	// Allocate the image data.
	if ((img = malloc(sizeof(*img))) == NULL)
		return false;

	// Set the output data pointer to the first byte of the decoded image.
	out->buf = &(*img)[0][0][0];

	// Create an array of pointers to each row of the image.
	uint8_t *row_pointers[out->height];

	for (uint16_t i = 0; i < out->height; i++)
		row_pointers[i] = &(*img)[i][0][0];

	// Read the image data.
	png_read_image(state->pngp, row_pointers);
	png_read_end(state->pngp, NULL);

	return true;
}

bool
png_load (const struct png_in *in, struct png_out *out)
{
	bool ret;
	struct state state;

	// Initialize the state structure.
	if (init(&state, in, out) == false)
		return false;

	// Return here on errors.
	if (setjmp(png_jmpbuf(state.pngp))) {
		ret = false;
	} else {
		ret = load(&state, out);
	}

	// Deinitialize the state structure.
	deinit(&state);

	// Free any allocated memory on an unsuccessful return.
	if (ret == false) {
		free(out->buf);
		out->buf = NULL;
	}

	return ret;
}
