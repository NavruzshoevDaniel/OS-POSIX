#include <semaphore.h>

void sem_try_init(sem_t *sem, int shared, int initial_value);
void sem_try_post(sem_t *sem);
void sem_try_wait(sem_t *sem);
void sem_try_destroy(sem_t *sem);
