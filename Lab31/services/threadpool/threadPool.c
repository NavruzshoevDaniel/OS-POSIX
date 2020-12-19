//
// Created by Daniel on 19.12.2020.
//

#include "threadPool.h"


int createThreadPool(int count, void *runnable, int *threadsId, pthread_t *poolThreads) {

    poolThreads = (pthread_t *) malloc(sizeof(pthread_t) * count);
    if (NULL == poolThreads) {
        printf("ERROR WHILE MALLOC createThreadPool\n");
        return -1;
    }

    threadsId = (int *) malloc(sizeof(int) * count);
    if (NULL == threadsId) {
        printf("ERROR WHILE MALLOC createThreadPool\n");
        free(poolThreads);
        return -1;
    }

    for (int i = 0; i < count; ++i) {

        threadsId[i] = i;

        if (pthread_create(&poolThreads[i], NULL, runnable, &threadsId[i]) > 0) {
            perror("Cannot create thread");
            for (int j = 0; j < i; j++) {
                pthread_cancel(poolThreads[j]);
            }
            for (int j = 0; j < i; j++) {
                pthread_join(poolThreads[j], NULL);
            }
            free(poolThreads);
            free(threadsId);
            return -1;
        }
    }
    return 0;
}

