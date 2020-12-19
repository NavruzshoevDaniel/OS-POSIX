//
// Created by Daniel on 19.12.2020.
//

#ifndef LAB31_PTHREADSERVICE_H
#define LAB31_PTHREADSERVICE_H

#include <stdbool.h>
#include "pthread.h"
#include "stdio.h"
#include "stdlib.h"

int initMutex(pthread_mutex_t *mutex);

int initCondVariable(pthread_cond_t *cond);

#endif //LAB31_PTHREADSERVICE_H
