#include <stdbool.h>
#include <pthread.h>

extern bool thread_create (pthread_t *t, void *(*tmain) (void *), void *arg);
extern bool thread_join (pthread_t t);

extern bool thread_cond_init (pthread_cond_t *c);
extern bool thread_cond_destroy (pthread_cond_t *c);

extern bool thread_cond_wait (pthread_cond_t *c, pthread_mutex_t *m);
extern bool thread_cond_signal (pthread_cond_t *c);
extern bool thread_cond_broadcast (pthread_cond_t *c);

extern bool thread_mutex_init (pthread_mutex_t *m);
extern bool thread_mutex_destroy (pthread_mutex_t *m);

extern bool thread_mutex_lock (pthread_mutex_t *m);
extern bool thread_mutex_unlock (pthread_mutex_t *m);
