#include "console_app_tools.h"
#include <pthread.h>

void mutex_try_init(pthread_mutex_t *mutex)
{
    pthread_mutexattr_t attrs;
    if (pthread_mutexattr_init(&attrs))
        throw_and_exit("pthread_mutexattr_init");
    if (pthread_mutexattr_settype(&attrs, PTHREAD_MUTEX_ERRORCHECK_NP))
        throw_and_exit("pthread_mutexattr_settype");
    if (pthread_mutex_init(mutex, &attrs))
        throw_and_exit("pthread_mutexattr_init");
}

void mutex_try_lock(pthread_mutex_t *mutex)
{
    if (pthread_mutex_lock(mutex))
        throw_and_exit("pthread_mutex_lock");
}

void mutex_try_unlock(pthread_mutex_t *mutex)
{
    if (pthread_mutex_unlock(mutex))
        throw_and_exit("pthread_mutex_unlock");
}

void mutex_try_destroy(pthread_mutex_t *mutex)
{
    if (pthread_mutex_destroy(mutex))
        throw_and_exit("pthread_mutex_destroy");
}