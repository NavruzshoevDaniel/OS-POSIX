#ifndef ERROR_HANDLED_MUTEX
#define ERROR_HANDLED_MUTEX

#include <pthread.h>

void mutex_try_init(pthread_mutex_t *mutex);
void mutex_try_lock(pthread_mutex_t *mutex);
void mutex_try_unlock(pthread_mutex_t *mutex);
void mutex_try_destroy(pthread_mutex_t *mutex);

#endif