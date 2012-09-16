#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <png.h>

#include "framerate.h"
#include "pngloader.h"
#include "xylist.h"

#define ZOOM_MAX	18
#define MAX_BITMAPS	1000			// Keep at most this many bitmaps in the cache

struct thread {
	struct xylist_req req;
	pthread_t thread;
	pthread_attr_t attr;
	pthread_mutex_t run_mutex;
	int running;
	void (*completed_callback)(void);
};

struct png_thread_resources {
	int fd;
	FILE *fp;
	void *rawbits;
	struct thread *t;
	png_structp png_ptr;
	png_infop info_ptr;
};

static struct xylist *bitmaps = NULL;
static struct xylist *threads = NULL;
static pthread_mutex_t bitmaps_mutex;
static pthread_mutex_t threads_mutex;

// Thread-local storage of resources:
static __thread struct png_thread_resources res = { -1, NULL, NULL, NULL, NULL, NULL };

static bool
viking_filename (char *buf, size_t buflen, unsigned int zoom, int tile_x, int tile_y)
{
	size_t len;
	char *home;

	// FIXME: horrible copypaste of code from viewport.c:
	unsigned int world_size = ((unsigned int)1) << (zoom + 8);

	if ((home = getenv("HOME")) == NULL) {
		return NULL;
	}
	if ((len = snprintf(buf, buflen, "%s/.viking-maps/t13s%uz0/%u/%u", home, 17 - zoom, tile_x, world_size / 256 - 1 - tile_y)) == buflen) {
		return false;
	}
	buf[len] = '\0';
	return true;
}

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

	pthread_mutex_lock(&threads_mutex);
	xylist_request(threads, req);
	pthread_mutex_unlock(&threads_mutex);
	return NULL;
}

static void
bitmap_destroy (void *data)
{
	free(data);
}

static void
thread_procure_on_cancel (void *data)
{
	(void)(data);

	if (res.fp != NULL) {
		fclose(res.fp);
	}
	else if (res.fd >= 0) {
		close(res.fd);
	}
	png_destroy_read_struct(&res.png_ptr, &res.info_ptr, NULL);
	pthread_mutex_destroy(&res.t->run_mutex);
	pthread_attr_destroy(&res.t->attr);
	free(res.rawbits);
	free(res.t);
}

static void *
thread_procure_main (void *data)
{
	int ret;
	int found = 0;
	unsigned int width;
	unsigned int height;
	char filename[100];

	// This runs as a separate thread. Load the image from disk and store
	// it into the bitmaps xylist when done.

	pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, NULL);
	pthread_detach(pthread_self());

	// Pointer to controlling structure. Ownership lies with thread:
	res.t = data;

	pthread_mutex_lock(&res.t->run_mutex);
	res.t->running = 1;
	pthread_mutex_unlock(&res.t->run_mutex);

	// Run this cleanup function if we're suddenly canceled:
	pthread_cleanup_push(thread_procure_on_cancel, NULL);

	if (!viking_filename(filename, sizeof(filename), res.t->req.zoom, res.t->req.xn, res.t->req.yn)) {
		goto exit;
	}
	// Get a file descriptor to the file. We just want a file descriptor to
	// associate with this thread so that we know what to clean up,
	// and not wait for the disk already, so we issue a nonblocking call:
	if ((res.fd = open(filename, O_RDONLY | O_NONBLOCK)) < 0) {
		goto exit;
	}
	// Now that we have the fd, make the fd properly blocking:
	fcntl(res.fd, F_SETFL, fcntl(res.fd, F_GETFL) & ~O_NONBLOCK);

	// We have the fd for our cleanup. From here on, allow sudden death:
	pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);

	// Now open an fp from the descriptor and load the image:
	// (this is the blocking part of the thread, we hope):
	if ((res.fp = fdopen(res.fd, "rb")) == NULL) {
		goto exit;
	}
	if (!load_png_file(&res.fp, &res.png_ptr, &res.info_ptr, &height, &width, &res.rawbits)) {
		goto exit;
	}
	if (res.fp != NULL) {
		fclose(res.fp);
		res.fp = NULL;
		res.fd = -1;
	}
	if (height != 256 || width != 256) {
		free(res.rawbits);
		res.rawbits = NULL;
		goto exit;
	}
	// Got tile, insert into bitmaps:
	found = 1;
	pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, NULL);
	pthread_mutex_lock(&bitmaps_mutex);
	ret = 0;
	if (bitmaps != NULL) {
		ret = xylist_insert_tile(bitmaps, res.t->req.zoom, res.t->req.xn, res.t->req.yn, res.rawbits);
	}
	pthread_mutex_unlock(&bitmaps_mutex);
	if (!ret) {
		free(res.rawbits);
		res.rawbits = NULL;
	}
	else if (res.t->completed_callback != NULL) {
		res.t->completed_callback();
	}

exit:	pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, NULL);
	pthread_cleanup_pop(0);
	if (res.fp != NULL) {
		fclose(res.fp);
		res.fp = NULL;
		res.fd = -1;
	}
	else if (res.fd >= 0) {
		close(res.fd);
		res.fd = -1;
	}
	// FIXME it's a bit ugly to call libpng functions from here:
	png_destroy_read_struct(&res.png_ptr, &res.info_ptr, NULL);

	// Really rudimentary rate-limiting to occupy this (zoom,xn,yn) slot a little longer,
	// ensurng that unavailable tiles aren't re-probed at every screen refresh:
	if (!found) {
		pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
		sleep(5);
		pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, NULL);
	}
	// We delete ourselves from the threads xylist. This will call
	// the destructor and detach our controlling structure.
	// Theoretically this is not us, but a newer tile that we just downloaded:
	pthread_mutex_lock(&threads_mutex);
	// Threads can be NULL if application is just shutting down:
	if (threads != NULL) {
		xylist_delete_tile(threads, res.t->req.zoom, res.t->req.xn, res.t->req.yn);
	}
	pthread_mutex_unlock(&threads_mutex);
	pthread_mutex_lock(&res.t->run_mutex);
	res.t->running = 0;
	pthread_mutex_unlock(&res.t->run_mutex);
	pthread_mutex_destroy(&res.t->run_mutex);
	pthread_attr_destroy(&res.t->attr);
	free(res.t);
	res.t = NULL;

	pthread_exit(NULL);
}

static void *
thread_procure (struct xylist_req *req)
{
	struct thread *t;

	// We have been asked to create a thread that loads the given bitmap
	// from disk and puts it in the bitmaps xylist.

	if ((t = malloc(sizeof(*t))) == NULL) {
		return NULL;
	}
	memcpy(&t->req, req, sizeof(*req));
	pthread_attr_init(&t->attr);
	pthread_mutex_init(&t->run_mutex, NULL);
	t->running = 0;
	t->completed_callback = framerate_request_refresh;

	// Start thread:
	pthread_create(&t->thread, &t->attr, &thread_procure_main, t);

	// Store pointer into threads xylist. The ownership (who deletes this)
	// lies with the thread itself, not with the xylist!
	return t;
}

static void
thread_destroy (void *data)
{
	struct thread *t = data;

	// The xylist does not own the pointer to the thread structure;
	// it is up to the thread to free it, we merely ask the thread to quit
	// and remove the pointer to its control structure:

	pthread_mutex_lock(&t->run_mutex);
	if (t->running) {
		pthread_cancel(t->thread);
		t->running = 0;
	}
	pthread_mutex_unlock(&t->run_mutex);
}

bool
bitmap_mgr_init (void)
{
	if ((bitmaps = xylist_create(0, ZOOM_MAX, MAX_BITMAPS, &bitmap_procure, &bitmap_destroy)) == NULL) {
		return false;
	}
	if ((threads = xylist_create(0, ZOOM_MAX, 20, &thread_procure, &thread_destroy)) == NULL) {
		xylist_destroy(&bitmaps);
		return false;
	}
	pthread_mutex_init(&bitmaps_mutex, NULL);
	pthread_mutex_init(&threads_mutex, NULL);
	return true;
}

void
bitmap_mgr_destroy (void)
{
	pthread_mutex_lock(&threads_mutex);
	xylist_destroy(&threads);
	pthread_mutex_unlock(&threads_mutex);

	pthread_mutex_lock(&bitmaps_mutex);
	xylist_destroy(&bitmaps);
	pthread_mutex_unlock(&bitmaps_mutex);

	pthread_mutex_destroy(&threads_mutex);
	pthread_mutex_destroy(&bitmaps_mutex);
}
