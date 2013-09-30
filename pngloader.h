#ifndef PNGLOADER_H
#define PNGLOADER_H

struct pngloader
{
	pthread_t thread;
	void (*on_completed)(struct pngloader *, void *);

	struct quadtree_req req;

	char *filename;
};

void pngloader_on_init (void);
void pngloader_main (void *data);
void pngloader_on_cancel (void);
void pngloader_on_exit (void);

#endif /* PNGLOADER_H */
