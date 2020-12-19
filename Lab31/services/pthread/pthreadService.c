//
// Created by Daniel on 19.12.2020.
//
#include "pthreadService.h"

void initMutex(pthread_mutex_t *mutex) {
    if (pthread_mutex_init(mutex, NULL) != 0) {
        perror("Cannot init mutex");
        exit(EXIT_FAILURE);
    }
}

void initCondVariable(pthread_cond_t *cond) {
    if (pthread_cond_init(cond, NULL) != 0) {
        perror("Cannot init condVar");
        exit(EXIT_FAILURE);
    }
}