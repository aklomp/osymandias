#pragma once

#include <pthread.h>

#include "quadtree.h"

struct pngloader
{
	pthread_t thread;
	void (*on_completed)(struct pngloader *, void *);

	struct quadtree_req req;
};

extern void pngloader_main (void *data);
