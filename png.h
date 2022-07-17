#pragma once

#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>

// Input structure: describes a blob of PNG data,
struct png_in {

	// Filename (or some other identifier) for errors/warnings/debugging.
	const char *name;

	// Pointer to the raw PNG data blob.
	const uint8_t *buf;

	// Length of the raw PNG data blob in bytes.
	size_t len;
};

// Output structure: describes a decoded blob of raw image pixel data.
struct png_out {

	// Pointer to the raw image pixel data blob.
	uint8_t *buf;

	// Height of the image in pixels.
	uint16_t height;

	// Width of the image in pixels.
	uint16_t width;

	// Number of color channels in the image pixel data.
	uint8_t channels;
};

extern bool png_load (const struct png_in *in, struct png_out *out);
