#include <GL/gl.h>

#include "texture_cache.h"

#define CACHE_CAPACITY	100

static struct cache *cache = NULL;

static void
destroy (union cache_data *data)
{
	glDeleteTextures(1, &data->u32);
}

uint32_t
texture_cache_search (const struct cache_node *in, struct cache_node *out)
{
	union cache_data *data;

	if ((data = cache_search(cache, in, out)) == NULL)
		return 0;

	return data->u32;
}

uint32_t
texture_cache_insert (const struct cache_node *loc, const void *rawbits)
{
	uint32_t id;

	glGenTextures(1, &id);
	glBindTexture(GL_TEXTURE_2D, id);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, 256, 256, 0, GL_RGB, GL_UNSIGNED_BYTE, rawbits);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

	cache_insert(cache, loc, &((union cache_data) { .u32 = id }));
	return id;
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
		.capacity = CACHE_CAPACITY,
		.destroy  = destroy,
	};

	return (cache = cache_create(&config)) != NULL;
}
