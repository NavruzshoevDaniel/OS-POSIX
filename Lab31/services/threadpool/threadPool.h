//
// Created by Daniel on 19.12.2020.
//

#ifndef LAB31_THREADPOOL_H
#define LAB31_THREADPOOL_H

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>

void initThread(pthread_t *thread, void *runnable, int *threadsId, int i);

pthread_t *createThreadPool(void *runnableThread, int *threadsId, int count);

#endif //LAB31_THREADPOOL_H
