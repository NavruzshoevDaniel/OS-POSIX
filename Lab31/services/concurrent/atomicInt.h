//
// Created by Daniel on 20.12.2020.
//

#ifndef LAB31_ATOMICINT_H
#define LAB31_ATOMICINT_H

#include "pthread.h"

void atomicIncrement(int *value, pthread_mutex_t *mutex);

void atomicDecrement(int *value, pthread_mutex_t *mutex);

#endif //LAB31_ATOMICINT_H
