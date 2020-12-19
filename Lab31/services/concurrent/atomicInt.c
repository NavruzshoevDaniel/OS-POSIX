//
// Created by Daniel on 20.12.2020.
//
#include "atomicInt.h"

void atomicIncrement(int *value, pthread_mutex_t *mutex) {
    pthread_mutex_lock(mutex);
    (*value)++;
    pthread_mutex_unlock(mutex);
}

void atomicDecrement(int *value, pthread_mutex_t *mutex) {
    pthread_mutex_lock(mutex);
    (*value)--;
    pthread_mutex_unlock(mutex);
}
