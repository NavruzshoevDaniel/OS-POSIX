#include "../console_app_tools.h"
#include <semaphore.h>

void sem_try_init(sem_t *sem, int shared, int initial_value)
{
    if (sem_init(sem, shared, initial_value))
        throw_and_exit("sem_init");
}

void sem_try_post(sem_t *sem)
{
    if (sem_post(sem))
        throw_and_exit("sem_post");
}

void sem_try_wait(sem_t *sem)
{
    if (sem_wait(sem))
        throw_and_exit("sem_wait");
}

void sem_try_destroy(sem_t *sem)
{
    if (sem_destroy(sem))
        throw_and_exit("sem_close");
}