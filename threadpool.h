#pragma once

struct threadpool;

// Create the threadpool.
// Returns a pointer to the allocated threadpool structure on success,
// NULL on failure.
struct threadpool *threadpool_create
(
	// Number of threads to create:
	size_t nthreads,

	// Run once when thread starts up:
	void (*on_init)(void),

	// Run when data is dequeued without being processed:
	void (*on_dequeue)(void *data),

	// Main thread routine:
	void (*routine)(void *data),

	// Run in signal handler when thread is canceled:
	void (*on_cancel)(void),

	// Run once when thread shuts down:
	void (*on_exit)(void)
);

// Enqueue the job specified by the opaque data pointer into the threadpool.
// Returns a job id larger than 0 if enqueuement succeeded, or 0 on failure.
int threadpool_job_enqueue (struct threadpool *const p, void *const data);

// Cancel the job with the given ID by signaling the thread. The signal handler
// routine calls the user-provided on_cancel() function, which is responsible
// for taking some action that causes the main routine to quit. This function
// does not wait for the main function to quit, so upon return there is no
// guarantee that the job slot is free.
// Returns true on success, false on error.
bool threadpool_job_cancel (struct threadpool *const p, int job_id);

// Destroy the threadpool structure and all associated resources:
void threadpool_destroy (struct threadpool **const p);
