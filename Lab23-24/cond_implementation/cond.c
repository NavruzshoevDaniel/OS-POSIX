#include "../console_app_tools.h"
#include <pthread.h>

void cond_try_init(pthread_cond_t *cond)
{
    if (pthread_cond_init(cond, NULL))
        throw_and_exit("pthread_cond_init");
}

void cond_try_signal(pthread_cond_t *cond)
{
    if (pthread_cond_signal(cond))
        throw_and_exit("pthread_mutex_lock");
}

void cond_try_broadcast(pthread_cond_t *cond)
{
    if (pthread_cond_broadcast(cond))
        throw_and_exit("pthread_mutex_broadcast");
}

void cond_try_wait(pthread_cond_t *cond, pthread_mutex_t *mutex)
{
    if (pthread_cond_wait(cond, mutex))
        throw_and_exit("pthread_cond_wait");
}

void cond_try_destroy(pthread_cond_t *cond)
{
    if (pthread_cond_destroy(cond))
        throw_and_exit("pthread_cond_destroy");
}