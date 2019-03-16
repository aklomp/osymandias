#pragma once

#include <stdbool.h>
#include <stddef.h>

struct threadpool;

// Create the threadpool.
// Returns a pointer to the allocated threadpool structure on success,
// NULL on failure.
struct threadpool *threadpool_create
(
	// Number of threads to create:
	size_t nthreads,

	// Run when data is dequeued without being processed:
	void (*on_dequeue)(void *data),

	// Main thread routine:
	void (*routine)(void *data)
);

// Enqueue the job specified by the opaque data pointer into the threadpool.
// Returns a job id larger than 0 if enqueuement succeeded, or 0 on failure.
int threadpool_job_enqueue (struct threadpool *const p, void *const data);

// Destroy the threadpool structure and all associated resources:
void threadpool_destroy (struct threadpool **const p);
