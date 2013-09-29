#ifndef PNGLOADER_H
#define PNGLOADER_H

struct pngloader
{
	pthread_t thread;
	void (*completed_callback)(void);

	struct quadtree **bitmaps;
	pthread_mutex_t *bitmaps_mutex;

	struct quadtree_req req;

	char *filename;
};

void pngloader_on_init (void);
void pngloader_main (void *data);
void pngloader_on_cancel (void);
void pngloader_on_exit (void);

#endif /* PNGLOADER_H */
