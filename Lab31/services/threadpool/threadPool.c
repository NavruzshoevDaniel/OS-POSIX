//
// Created by Daniel on 19.12.2020.
//

#include "threadPool.h"


void initThread(pthread_t *thread, void *runnable, int *threadsId, int i) {
    if (pthread_create(thread, NULL, runnable, &threadsId[i]) > 0) {
        perror("Cannot create thread");
        exit(EXIT_FAILURE);
    }
}

pthread_t *createThreadPool(void *runnableThread, int *threadsId, int count) {
    pthread_t *threads = malloc(sizeof(pthread_t) * count);

    for (int i = 0; i < count; ++i) {
        threadsId[i] = i;
        initThread(&threads[i], runnableThread, threadsId, i);
    }
    return threads;
}

