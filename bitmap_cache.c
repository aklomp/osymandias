#include <stdlib.h>
#include <string.h>
#include <pthread.h>

#include "gui/framerate.h"
#include "bitmap_cache.h"
#include "threadpool.h"
#include "pngloader.h"

#define CACHE_SIZE		1000
#define THREADPOOL_JOBS		40
#define THREADPOOL_THREADS	8

static struct cache      *cache = NULL;
static struct threadpool *tpool = NULL;
static pthread_mutex_t    mutex = PTHREAD_MUTEX_INITIALIZER;

static void
process (void *data)
{
	void *rawbits;
	struct cache_node *req = data;

	if ((rawbits = pngloader_main(req)) == NULL)
		return;

	pthread_mutex_lock(&mutex);
	cache_insert(cache, req, &((union cache_data) { .ptr = rawbits }));
	pthread_mutex_unlock(&mutex);

	framerate_repaint();
}

static void
destroy (union cache_data *data)
{
	free(data->ptr);
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
	cache_insert(cache, loc, &((union cache_data) { .ptr = NULL }));
}

void *
bitmap_cache_search (const struct cache_node *in, struct cache_node *out)
{
	union cache_data *data = cache_search(cache, in, out);

	// Return successfully if valid data was found at the requested level:
	if (data != NULL && data->ptr != NULL && in->zoom == out->zoom)
		return data->ptr;

	// If no node was found or it is at a different zoom level than
	// requested, then start a threadpool job to procure the target:
	if (data == NULL || in->zoom != out->zoom)
		procure(in);

	// If we got no data back at all, the cache is empty:
	if (data == NULL)
		return NULL;

	// If we got back some non-NULL data, it is a valid bitmap at some
	// lower zoom level. Better than nothing:
	if (data->ptr != NULL)
		return data->ptr;

	// We got back a valid data pointer but with a NULL member. This
	// indicates that we landed on a tile that is currently being procured.
	// Move up one zoom layer and retry:
	struct cache_node up = *out;

	return cache_node_up(&up) ? bitmap_cache_search(&up, out) : NULL;
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
		.capacity = CACHE_SIZE,
		.destroy  = destroy,
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
