#include <string.h>
#include <stdio.h>
#include <errno.h>

#include "thread.h"

#define CHECK(FN, ...)					\
	{						\
		const int err = FN(__VA_ARGS__);	\
							\
		if (err == 0)				\
			return true;			\
							\
		print_error(#FN, err);			\
		return false;				\
	}

static void
print_error (const char *func, const int err)
{
	fprintf(stderr, "%s: %s\n", func, strerror(err));
}

bool
thread_create (pthread_t *t, void *(*tmain) (void *), void *arg)
{
	if (t == NULL || tmain == NULL) {
		print_error(__func__, EINVAL);
		return false;
	}

	CHECK(pthread_create, t, NULL, tmain, arg)
}

bool
thread_join (pthread_t t)
{
	CHECK(pthread_join, t, NULL)
}

bool
thread_cond_init (pthread_cond_t *c)
{
	if (c == NULL) {
		print_error(__func__, EINVAL);
		return false;
	}

	CHECK(pthread_cond_init, c, NULL)
}

bool
thread_cond_destroy (pthread_cond_t *c)
{
	if (c == NULL) {
		print_error(__func__, EINVAL);
		return false;
	}

	CHECK(pthread_cond_destroy, c)
}

bool
thread_cond_wait (pthread_cond_t *c, pthread_mutex_t *m)
{
	if (c == NULL || m == NULL) {
		print_error(__func__, EINVAL);
		return false;
	}

	CHECK(pthread_cond_wait, c, m)
}

bool
thread_cond_signal (pthread_cond_t *c)
{
	if (c == NULL) {
		print_error(__func__, EINVAL);
		return false;
	}

	CHECK(pthread_cond_signal, c)
}

bool
thread_cond_broadcast (pthread_cond_t *c)
{
	if (c == NULL) {
		print_error(__func__, EINVAL);
		return false;
	}

	CHECK(pthread_cond_broadcast, c)
}

bool
thread_mutex_init (pthread_mutex_t *m)
{
	if (m == NULL) {
		print_error(__func__, EINVAL);
		return false;
	}

	CHECK(pthread_mutex_init, m, NULL)
}

bool
thread_mutex_destroy (pthread_mutex_t *m)
{
	if (m == NULL) {
		print_error(__func__, EINVAL);
		return false;
	}

	CHECK(pthread_mutex_destroy, m)
}

bool
thread_mutex_lock (pthread_mutex_t *m)
{
	if (m == NULL) {
		print_error(__func__, EINVAL);
		return false;
	}

	CHECK(pthread_mutex_lock, m)
}

bool
thread_mutex_unlock (pthread_mutex_t *m)
{
	if (m == NULL) {
		print_error(__func__, EINVAL);
		return false;
	}

	CHECK(pthread_mutex_unlock, m)
}
