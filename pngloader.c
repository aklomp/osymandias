#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <png.h>

bool
load_png_file (const char *const filename, unsigned int *height, unsigned int *width, void **rawbits)
{
	FILE *fp;
	png_structp png_ptr = NULL;
	png_infop info_ptr = NULL;
	bool ret = false;

	*rawbits = NULL;

	if (!(fp = fopen(filename, "rb"))) {
		goto exit_0;
	}
	// TODO: we don't handle errors; what happens when they occur?
	if ((png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL)) == NULL) {
		goto exit_1;
	}
	if ((info_ptr = png_create_info_struct(png_ptr)) == NULL) {
		goto exit_2;
	}
	if (setjmp(png_jmpbuf(png_ptr))) {
		goto exit_2;
	}
	// Check that the file is a PNG; if so, init the i/o functions:
	{
		char buf[8];

		if (fread(buf, 1, sizeof(buf), fp) != sizeof(buf)) {
			goto exit_2;
		}
		if (png_sig_cmp((png_bytep)buf, 0, sizeof(buf))) {
			goto exit_2;
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
		goto exit_2;
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

exit_2:	png_destroy_read_struct(&png_ptr, &info_ptr, NULL);
	if (ret == false) {
		free(*rawbits);
		*rawbits = NULL;
	}
exit_1:	fclose(fp);
exit_0:	return ret;
}
