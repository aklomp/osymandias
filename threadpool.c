#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <limits.h>
#include <pthread.h>

#include "util.h"

enum job_slot_status {
	SLOT_FREE,
	SLOT_WAIT,
	SLOT_BUSY
};

struct job {
	int id;
	enum job_slot_status status;
	void *data;
};

struct thread {
	pthread_t id;
};

struct threadpool {
	size_t nthreads;
	struct job *jobs;
	struct thread **threads;

	// User-provided routines:
	void (*routine)(void *data);

	int job_counter;
	size_t free_counter;
	bool shutdown;
	pthread_cond_t cond;
	pthread_mutex_t cond_mutex;
};

// Check a pthread function for errors, print error message.
static void
check (const int ret)
{
	if (ret != 0)
		fprintf(stderr, "pthread: %s\n", strerror(ret));
}

// Check a pthread function for errors, return error status.
static bool
failed (const int ret)
{
	check(ret);
	return ret != 0;
}

static struct job *
job_first_free (const struct threadpool *const p)
{
	// Quick check: no jobs free?
	if (p->free_counter == 0)
		return NULL;

	// Find next free slot in jobs table:
	FOREACH_NELEM (p->jobs, p->nthreads, j)
		if (j->status == SLOT_FREE)
			return j;

	return NULL;
}

static struct job *
job_first_waiting (const struct threadpool *const p)
{
	// Quick check: if all slots are free, there are no waiting jobs:
	if (p->free_counter == p->nthreads)
		return NULL;

	// Find first eligible job in jobs table:
	FOREACH_NELEM (p->jobs, p->nthreads, j)
		if (j->status == SLOT_WAIT)
			return j;

	return NULL;
}

static inline void
slot_set_wait (struct threadpool *const p, struct job *const j)
{
	j->status = SLOT_WAIT;
	p->free_counter--;
}

static inline void
slot_set_free (struct threadpool *const p, struct job *const j)
{
	j->status = SLOT_FREE;
	p->free_counter++;
}

static void *
thread_main (void *data)
{
	struct threadpool *p = data;

	while (p->shutdown == false) {
		struct job *job;

		// The pthread_cond_wait() must run under a locked mutex
		// (it does its own internal unlocking/relocking):
		if (failed(pthread_mutex_lock(&p->cond_mutex)))
			break;

		// Wait for predicate to change, allow spurious wakeups:
		while ((job = job_first_waiting(p)) == NULL) {
			if (failed(pthread_cond_wait(&p->cond, &p->cond_mutex)) || p->shutdown) {
				check(pthread_mutex_unlock(&p->cond_mutex));
				return NULL;
			}
		}

		// The condition mutex is now locked, claim the job:
		job->status = SLOT_BUSY;

		// Unlock the condition mutex to grant access to the job structure:
		check(pthread_mutex_unlock(&p->cond_mutex));

		// Run user-provided routine on data:
		p->routine(job->data);

		// When done, mark the job slot as FREE again:
		check(pthread_mutex_lock(&p->cond_mutex));
		slot_set_free(p, job);
		check(pthread_mutex_unlock(&p->cond_mutex));
	}

	return NULL;
}

static struct thread *
thread_create (struct threadpool *const p)
{
	struct thread *t;

	if ((t = malloc(sizeof(*t))) == NULL) {
		return NULL;
	}

	if (failed(pthread_create(&t->id, NULL, thread_main, p))) {
		free(t);
		return NULL;
	}

	return t;
}

static void
thread_destroy (struct thread **const t)
{
	if (t == NULL || *t == NULL) {
		return;
	}

	check(pthread_join((*t)->id, NULL));

	free(*t);
	*t = NULL;
}

struct threadpool *
threadpool_create (size_t nthreads,
	void (*routine)(void *data))
{
	struct thread *t;
	struct threadpool *const p = malloc(sizeof(*p));

	if (p == NULL) {
		goto err_0;
	}
	if ((p->jobs = malloc(sizeof(struct job) * nthreads)) == NULL) {
		goto err_1;
	}
	if ((p->threads = malloc(sizeof(struct thread *) * nthreads)) == NULL) {
		goto err_2;
	}
	p->nthreads = nthreads;
	p->routine = routine;
	p->shutdown = false;
	p->free_counter = nthreads;
	p->job_counter = 1;

	if (failed(pthread_mutex_init(&p->cond_mutex, NULL))) {
		goto err_3;
	}

	if (failed(pthread_cond_init(&p->cond, NULL))) {
		goto err_4;
	}

	// Initialize job queue to empty state:
	FOREACH_NELEM (p->jobs, p->nthreads, j) {
		j->id = 0;
		j->data = NULL;
		j->status = SLOT_FREE;
	}

	// Create individual threads; they start running immediately:
	FOREACH_NELEM (p->threads, p->nthreads, q) {
		if ((t = thread_create(p)) != NULL) {
			*q = t;
			continue;
		}
		// Error. Backtrack, destroy previously created threads:
		for (struct thread **s = q - 1; s >= p->threads; s--) {
			thread_destroy(s);
		}
		goto err_5;
	}
	return p;

err_5:	check(pthread_cond_destroy(&p->cond));
err_4:	check(pthread_mutex_destroy(&p->cond_mutex));
err_3:	free(p->threads);
err_2:	free(p->jobs);
err_1:	free(p);
err_0:	return NULL;
}

void
threadpool_destroy (struct threadpool **const p)
{
	if (p == NULL || *p == NULL) {
		return;
	}

	// To be nice, broadcast shutdown message to all waiting threads:
	check(pthread_mutex_lock(&(*p)->cond_mutex));
	(*p)->shutdown = true;
	check(pthread_cond_broadcast(&(*p)->cond));

	// This should have taken care of the threads that were waiting in
	// pthread_cond_wait(). While still holding the mutex, dequeue all
	// pending jobs:
	FOREACH_NELEM ((*p)->jobs, (*p)->nthreads, j)
		if (j->status == SLOT_WAIT)
			slot_set_free(*p, j);

	// Release the mutex and join with all threads:
	check(pthread_mutex_unlock(&(*p)->cond_mutex));
	FOREACH_NELEM ((*p)->threads, (*p)->nthreads, t)
		thread_destroy(t);

	check(pthread_mutex_destroy(&(*p)->cond_mutex));
	check(pthread_cond_destroy(&(*p)->cond));

	free((*p)->threads);
	free((*p)->jobs);
	free(*p);
	*p = NULL;
}

int
threadpool_job_enqueue (struct threadpool *const p, void *const data)
{
	struct job *j;

	if (p == NULL) {
		return 0;
	}

	// Going to read/alter the predicate; must lock:
	if (failed(pthread_mutex_lock(&p->cond_mutex)))
		return 0;

	if ((j = job_first_free(p)) == NULL) {
		check(pthread_mutex_unlock(&p->cond_mutex));
		return 0;
	}

	// Fill slot with job:
	j->data = data;
	slot_set_wait(p, j);
	int job_id = j->id = p->job_counter;

	// Increment job counter, check for overflow, guarantee always > 0:
	p->job_counter = (p->job_counter == INT_MAX) ? 1 : p->job_counter + 1;

	// Notify a waiting thread about the new task;
	// according to the manpage, it's legal and preferred to do this while
	// holding the mutex:
	check(pthread_cond_signal(&p->cond));

	check(pthread_mutex_unlock(&p->cond_mutex));
	return job_id;
}
