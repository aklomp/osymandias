#include <GL/gl.h>

#include "texture_cache.h"

#define CACHE_CAPACITY	200

static struct cache *cache = NULL;

static void
on_destroy (void *data)
{
	struct texture_cache *tex = data;

	glDeleteTextures(1, &tex->id);
}

const struct texture_cache *
texture_cache_search (const struct cache_node *in, struct cache_node *out)
{
	return cache_search(cache, in, out);
}

const struct texture_cache *
texture_cache_insert (const struct cache_node *loc, const struct bitmap_cache *bitmap)
{
	struct texture_cache tex = {
		.coords = bitmap->coords,
	};

	glGenTextures(1, &tex.id);
	glBindTexture(GL_TEXTURE_2D, tex.id);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, 256, 256, 0, GL_RGB, GL_UNSIGNED_BYTE, bitmap->rgb);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

	return cache_insert(cache, loc, &tex);
}

void
texture_cache_destroy (void)
{
	cache_destroy(cache);
}

bool
texture_cache_create (void)
{
	const struct cache_config config = {
		.capacity  = CACHE_CAPACITY,
		.destroy   = on_destroy,
		.entrysize = sizeof (struct texture_cache),
	};

	return (cache = cache_create(&config)) != NULL;
}
