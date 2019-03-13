#pragma once

#include <stdbool.h>
#include <stddef.h>

// Opaque threadpool state structure.
struct threadpool;

// Threadpool config structure.
struct threadpool_config {

	// Main thread routine.
	void (* process)(void *data);

	struct {

		// Size of the job queue.
		size_t jobs;

		// Number of worker threads to create.
		size_t threads;
	} num;
};

// Create the threadpool.
// Returns a pointer to the allocated threadpool structure on success,
// NULL on failure.
extern struct threadpool *threadpool_create (const struct threadpool_config *config);

// Enqueue the job specified by the opaque data pointer into the threadpool.
// Returns a job id larger than 0 if enqueuement succeeded, or 0 on failure.
extern int threadpool_job_enqueue (struct threadpool *const p, void *const data);

// Destroy the threadpool structure and all associated resources:
extern void threadpool_destroy (struct threadpool **const p);
