//
// Created by Daniel on 19.12.2020.
//

#ifndef LAB31_THREADPOOL_H
#define LAB31_THREADPOOL_H

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>

int createThreadPool(int count, void *runnable, int *threadsId, pthread_t **poolThreads);

#endif //LAB31_THREADPOOL_H
