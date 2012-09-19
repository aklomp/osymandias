#ifndef PNGLOADER_H
#define PNGLOADER_H

struct pngloader_node
{
	int running;
	pthread_t *thread;
};

struct pngloader
{
	pthread_t thread;
	void (*completed_callback)(void);
	pthread_mutex_t *running_mutex;

	struct xylist **bitmaps;
	pthread_mutex_t *bitmaps_mutex;

	struct xylist_req req;
	struct pngloader_node *n;
};

void *pngloader_main (void *data);

#endif /* PNGLOADER_H */
