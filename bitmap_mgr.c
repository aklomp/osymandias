#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <pthread.h>

#include "framerate.h"
#include "xylist.h"
#include "pngloader.h"

#define ZOOM_MAX	18
#define MAX_BITMAPS	1000			// Keep at most this many bitmaps in the cache

static struct xylist *bitmaps = NULL;
static struct xylist *threadlist = NULL;
static pthread_mutex_t bitmaps_mutex;
static pthread_mutex_t threadlist_mutex;
static pthread_mutex_t running_mutex;

void *
bitmap_request (struct xylist_req *req)
{
	void *data;

	pthread_mutex_lock(&bitmaps_mutex);
	data = xylist_request(bitmaps, req);
	pthread_mutex_unlock(&bitmaps_mutex);
	return data;
}

static void *
bitmap_procure (struct xylist_req *req)
{
	// We have been asked to procure a bitmap. We can't do this fast enough
	// for this refresh, so we return NULL and shoot off a thread.
	// We use the 'thread' structure to save the thread data, and to check if
	// we already have a thread running for this tile.

	// Look up the running thread with xylist_request. If it does not exist,
	// it will be procured by xylist_request by indirectly calling back on
	// thread_procure(). Our work here is done.

	pthread_mutex_lock(&threadlist_mutex);
	xylist_request(threadlist, req);
	pthread_mutex_unlock(&threadlist_mutex);
	return NULL;
}

static void
bitmap_destroy (void *data)
{
	free(data);
}

static void *
thread_procure (struct xylist_req *req)
{
	struct pngloader *p;
	struct pngloader_node *n;
	pthread_attr_t attr;

	// We have been asked to create a thread that loads the given bitmap
	// from disk and puts it in the bitmaps xylist.

	if ((p = malloc(sizeof(*p))) == NULL) {
		return NULL;
	}
	if ((n = malloc(sizeof(*n))) == NULL) {
		free(p);
		return NULL;
	}
	// The pngloader structure is owned by the thread.
	// It is responsible for free()'ing it.
	p->bitmaps = &bitmaps;
	p->bitmaps_mutex = &bitmaps_mutex;
	memcpy(&p->req, req, sizeof(*req));
	p->completed_callback = framerate_request_refresh;
	p->running_mutex = &running_mutex;
	p->n = n;

	// The pngloader_node structure is owned by us, the threadlist.
	// We will free it in thread_destroy().
	n->running = 1;
	n->thread = &p->thread;

	pthread_attr_init(&attr);
	pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);

	// Start thread:
	pthread_create(&p->thread, &attr, &pngloader_main, p);
	pthread_attr_destroy(&attr);

	// Store node in xylist:
	return n;
}

static void
thread_destroy (void *data)
{
	struct pngloader_node *n = data;

	// Free the node pointer. If we can acquire the lock, we are guaranteed
	// that the thread is either waiting for the lock, or already dead.
	pthread_mutex_lock(&running_mutex);
	// pthread_cancel() segfaults horribly if a thread is not running,
	// hence the check:
	if (n->running) {
		pthread_cancel(*n->thread);
	}
	pthread_mutex_unlock(&running_mutex);
	free(n);
}

bool
bitmap_mgr_init (void)
{
	if ((bitmaps = xylist_create(0, ZOOM_MAX, MAX_BITMAPS, &bitmap_procure, &bitmap_destroy)) == NULL) {
		return false;
	}
	if ((threadlist = xylist_create(0, ZOOM_MAX, 200, &thread_procure, &thread_destroy)) == NULL) {
		xylist_destroy(&bitmaps);
		return false;
	}
	pthread_mutex_init(&bitmaps_mutex, NULL);
	pthread_mutex_init(&threadlist_mutex, NULL);
	pthread_mutex_init(&running_mutex, NULL);
	return true;
}

void
bitmap_mgr_destroy (void)
{
	pthread_mutex_lock(&threadlist_mutex);
	xylist_destroy(&threadlist);
	pthread_mutex_unlock(&threadlist_mutex);

	pthread_mutex_lock(&bitmaps_mutex);
	xylist_destroy(&bitmaps);
	pthread_mutex_unlock(&bitmaps_mutex);

	pthread_mutex_destroy(&running_mutex);
	pthread_mutex_destroy(&threadlist_mutex);
	pthread_mutex_destroy(&bitmaps_mutex);
}
