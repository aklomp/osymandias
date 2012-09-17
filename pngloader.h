#ifndef PNGLOADER_H
#define PNGLOADER_H

struct pngloader
{
	pthread_t thread;
	void (*completed_callback)(void);
	int running;
	pthread_mutex_t *running_mutex;

	struct xylist **bitmaps;
	pthread_mutex_t *bitmaps_mutex;

	struct xylist **threadlist;
	pthread_mutex_t *threadlist_mutex;

	struct xylist_req req;
};

void *pngloader_main (void *data);

#endif /* PNGLOADER_H */
