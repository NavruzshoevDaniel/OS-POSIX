//
// Created by Daniel on 19.12.2020.
//

#ifndef LAB31_PTHREADSERVICE_H
#define LAB31_PTHREADSERVICE_H

#include "pthread.h"
#include "stdio.h"
#include "stdlib.h"

void initMutex(pthread_mutex_t *mutex);

void initCondVariable(pthread_cond_t *cond);

#endif //LAB31_PTHREADSERVICE_H
