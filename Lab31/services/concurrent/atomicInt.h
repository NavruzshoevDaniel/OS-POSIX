//
// Created by Daniel on 20.12.2020.
//

#ifndef LAB31_ATOMICINT_H
#define LAB31_ATOMICINT_H

#include "pthread.h"

int incrementAndGet(int value, pthread_mutex_t *mutex);

int decrementAndGet(int value, pthread_mutex_t *mutex);

#endif //LAB31_ATOMICINT_H
