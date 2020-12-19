//
// Created by Daniel on 19.12.2020.
//
#include "pthreadService.h"

int initMutex(pthread_mutex_t *mutex) {
    if (pthread_mutex_init(mutex, NULL) != 0) {
        perror("Cannot init mutex");
        return -1;
    }
    return 0;
}

int initCondVariable(pthread_cond_t *cond) {
    if (pthread_cond_init(cond, NULL) != 0) {
        perror("Cannot init condVar");
        return -1;
    }
    return 0;
}

