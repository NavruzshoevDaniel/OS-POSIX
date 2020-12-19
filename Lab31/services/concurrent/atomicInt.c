//
// Created by Daniel on 20.12.2020.
//
#include "atomicInt.h"

int incrementAndGet(int value, pthread_mutex_t *mutex) {
    pthread_mutex_lock(mutex);
    value++;
    pthread_mutex_unlock(mutex);
    return value;
}

int decrementAndGet(int value, pthread_mutex_t *mutex){
    pthread_mutex_lock(mutex);
    value--;
    pthread_mutex_unlock(mutex);
    return value;
}
