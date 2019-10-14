#include <stdlib.h>
#include <string.h>
#include <pthread.h>

#include "gui/framerate.h"
#include "bitmap_cache.h"
#include "globe.h"
#include "threadpool.h"
#include "pngloader.h"

#define CACHE_SIZE		1000
#define THREADPOOL_JOBS		40
#define THREADPOOL_THREADS	8

static struct cache      *cache = NULL;
static struct threadpool *tpool = NULL;
static pthread_mutex_t    mutex = PTHREAD_MUTEX_INITIALIZER;

void
bitmap_cache_insert (const struct cache_node *loc, void *rgb)
{
	struct bitmap_cache bitmap = { .rgb = rgb };

	// Calculate 3D sphere xyz coordinates for this tile:
	globe_map_tile(loc, &bitmap.coords);

	// Insert the cache data structure into the bitmap cache:
	pthread_mutex_lock(&mutex);
	cache_insert(cache, loc, &bitmap);
	pthread_mutex_unlock(&mutex);

	// Ask for a redraw of the viewport:
	framerate_repaint();
}

static void
process (void *data)
{
	void *rgb;
	struct cache_node *req = data;

	// Store rawbits data pointer into cache data structure if found:
	if ((rgb = pngloader_main(req)) != NULL)
		bitmap_cache_insert(req, rgb);
}

static void
on_destroy (void *data)
{
	struct bitmap_cache *bitmap = data;

	free(bitmap->rgb);
}

static void
procure (const struct cache_node *loc)
{
	// Enqueue a job in the threadpool:
	if (threadpool_job_enqueue(tpool, (void *) loc) == false)
		return;

	// Insert a placeholder node into the cache to tell the system that
	// there is already a lookup in progress for this node. The node will
	// be overwritten by the thread when it is done. Until then, it acts as
	// a "tombstone", preventing multiple requeues of the same job:
	cache_insert(cache, loc, &(struct bitmap_cache) { .rgb = NULL });
}

const struct bitmap_cache *
bitmap_cache_search (const struct cache_node *in, struct cache_node *out)
{
	bool procuring = false;
	struct cache_node level = *in;
	const struct bitmap_cache *data;

	while (true) {

		// Search for valid data at the current level. If there is no
		// data at all, the cache is empty from here on down:
		if ((data = cache_search(cache, &level, out)) == NULL)
			break;

		// If we got back non-NULL pixels, it is a valid bitmap:
		if (data->rgb != NULL)
			break;

		// We got back a valid data pointer but with NULL pixels. This
		// indicates that the tile we landed on is currently being
		// procured. Check if the procurement is for the tile we want:
		if (in->zoom == out->zoom)
			procuring = true;

		// Move up one zoom layer and retry:
		if (cache_node_up(&level) == false) {
			data = NULL;
			break;
		}
	}

	// If no node was found or it is at a different zoom level than
	// requested, then start a threadpool job to procure the target:
	if (procuring == false && (data == NULL || in->zoom != out->zoom))
		procure(in);

	return data;
}

void
bitmap_cache_lock (void)
{
	pthread_mutex_lock(&mutex);
}

void
bitmap_cache_unlock (void)
{
	pthread_mutex_unlock(&mutex);
}

void
bitmap_cache_destroy (void)
{
	threadpool_destroy(tpool);
	cache_destroy(cache);
}

bool
bitmap_cache_create (void)
{
	const struct cache_config cache_config = {
		.capacity  = CACHE_SIZE,
		.destroy   = on_destroy,
		.entrysize = sizeof (struct bitmap_cache),
	};

	const struct threadpool_config threadpool_config = {
		.process = process,
		.jobsize = sizeof (struct cache_node),
		.num = {
			.jobs    = THREADPOOL_JOBS,
			.threads = THREADPOOL_THREADS,
		},
	};

	if ((cache = cache_create(&cache_config)) == NULL)
		return false;

	if ((tpool = threadpool_create(&threadpool_config)) == NULL) {
		cache_destroy(cache);
		return false;
	}

	return true;
}
