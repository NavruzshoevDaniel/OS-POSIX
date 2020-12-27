//
// Created by Daniel on 19.12.2020.
//

#include "threadPool.h"


int createThreadPool(int count, void *runnable, int *threadsId, pthread_t **poolThreads) {
    pthread_t *newPoolThreads;
    newPoolThreads = (pthread_t *) malloc(sizeof(pthread_t) * count);
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


int joinThreadPool(pthread_t *poolThreads, int sizeThreadPool) {
    for (int i = 0; i < sizeThreadPool; i++) {
        if (pthread_join(poolThreads[i], NULL) != 0) {
            printf("joinThreadPool: error while join pthread i=%d\n", i);
        }
    }
}

int cancelThreadPool(pthread_t *poolThreads, int sizeThreadPool) {
    for (int i = 0; i < sizeThreadPool; i++) {
        if (pthread_cancel(poolThreads[i]) != 0) {
            printf("cancelThreadPool: error while cancel pthread i=%d\n", i);
        }
    }
}

