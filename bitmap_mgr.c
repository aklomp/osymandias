#include <stdlib.h>
#include <string.h>
#include <pthread.h>

#include "gui/framerate.h"
#include "quadtree.h"
#include "threadpool.h"
#include "pngloader.h"

#define ZOOM_MAX	18
#define MAX_BITMAPS	1000			// Keep at most this many bitmaps in the cache
#define THREADPOOL_SIZE	30

static struct quadtree *bitmaps = NULL;
static struct quadtree *threadlist = NULL;
static struct threadpool *threadpool = NULL;
static pthread_mutex_t bitmaps_mutex;
static pthread_mutex_t running_mutex;
static pthread_attr_t attr_detached;

int
bitmap_request (struct quadtree_req *req)
{
	pthread_mutex_lock(&bitmaps_mutex);
	int ret = quadtree_request(bitmaps, req);
	pthread_mutex_unlock(&bitmaps_mutex);
	return ret;
}

void
bitmap_zoom_change (const int zoomlevel)
{
	(void)zoomlevel;
}

static void *
bitmap_procure (struct quadtree_req *req)
{
	// We have been asked to procure a bitmap. We can't do this fast enough
	// for this refresh, so we shoot off a threadpool job and return NULL.
	// The thread will fetch a bitmap from disk and insert it into the
	// bitmaps list when it's done, thus indirectly fetching the request
	// for future lookups. Right now, we're done.
	// We use the 'threadlist' quadtree to hold the job IDs, and to check
	// if we already have a thread running for this tile. Think of it as a
	// spatially aware job queue.

	pthread_mutex_lock(&running_mutex);
	quadtree_request(threadlist, req);
	pthread_mutex_unlock(&running_mutex);
	return NULL;
}

static void
bitmap_destroy (void *data)
{
	free(data);
}

static void
process (void *job)
{
	int stored;
	struct quadtree_req *req = job;
	void *rawbits = pngloader_main(req);

	// Insert bitmap into quadtree:
	pthread_mutex_lock(&bitmaps_mutex);
	stored = quadtree_data_insert(bitmaps, req, rawbits);
	pthread_mutex_unlock(&bitmaps_mutex);

	if (!stored) bitmap_destroy(rawbits);

	// Remove thread from list of running procure threads:
	pthread_mutex_lock(&running_mutex);
	quadtree_data_insert(threadlist, req, NULL);
	pthread_mutex_unlock(&running_mutex);

	framerate_repaint();
}

static void *
thread_procure (struct quadtree_req *req)
{
	if (threadpool_job_enqueue(threadpool, req))
		return (void *) 1;

	return NULL;
}

static void
thread_destroy (void *data)
{
	(void) data;
}

bool
bitmap_mgr_init (void)
{
	if ((bitmaps = quadtree_create(MAX_BITMAPS, &bitmap_procure, &bitmap_destroy)) == NULL)
		goto err_0;

	if ((threadlist = quadtree_create(200, &thread_procure, &thread_destroy)) == NULL)
		goto err_1;

	const struct threadpool_config config = {
		.process = process,
		.jobsize = sizeof (struct quadtree_req),
		.num = {
			.jobs    = THREADPOOL_SIZE,
			.threads = THREADPOOL_SIZE,
		},
	};

	if ((threadpool = threadpool_create(&config)) == NULL)
		goto err_2;

	pthread_mutex_init(&bitmaps_mutex, NULL);
	pthread_mutex_init(&running_mutex, NULL);
	pthread_attr_init(&attr_detached);
	pthread_attr_setdetachstate(&attr_detached, PTHREAD_CREATE_DETACHED);
	return true;

err_2:	quadtree_destroy(&threadlist);
err_1:	quadtree_destroy(&bitmaps);
err_0:	return false;
}

void
bitmap_mgr_destroy (void)
{
	threadpool_destroy(threadpool);

	quadtree_destroy(&threadlist);

	pthread_mutex_lock(&bitmaps_mutex);
	quadtree_destroy(&bitmaps);
	pthread_mutex_unlock(&bitmaps_mutex);

	pthread_attr_destroy(&attr_detached);
	pthread_mutex_destroy(&running_mutex);
	pthread_mutex_destroy(&bitmaps_mutex);
}
