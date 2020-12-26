//
// Created by Daniel on 19.12.2020.
//

#include "threadPool.h"


int createThreadPool(int count, void *runnable, int *threadsId, pthread_t **poolThreads) {

    pthread_t *newPoolThreads = (pthread_t *) malloc(sizeof(pthread_t) * count);
    if (NULL == newPoolThreads) {
        printf("ERROR WHILE MALLOC createThreadPool\n");
        return -1;
    }

    threadsId = (int *) malloc(sizeof(int) * count);
    if (NULL == threadsId) {
        printf("ERROR WHILE MALLOC createThreadPool\n");
        free(newPoolThreads);
        return -1;
    }

    for (int i = 0; i < count; ++i) {

        threadsId[i] = i;

        if (pthread_create(&newPoolThreads[i], NULL, runnable, &threadsId[i]) > 0) {
            perror("Cannot create thread");
            for (int j = 0; j < i; j++) {
                pthread_cancel(newPoolThreads[j]);
            }
            for (int j = 0; j < i; j++) {
                pthread_join(newPoolThreads[j], NULL);
            }
            free(newPoolThreads);
            free(threadsId);
            return -1;
        }
    }
    *poolThreads = newPoolThreads;
    return 0;
}

