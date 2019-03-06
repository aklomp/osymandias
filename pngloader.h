#pragma once

struct pngloader
{
	pthread_t thread;
	void (*on_completed)(struct pngloader *, void *);

	struct quadtree_req req;
};

void pngloader_on_init (void);
void pngloader_main (void *data);
void pngloader_on_cancel (void);
void pngloader_on_exit (void);
