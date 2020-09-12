#include "pthread.h"
#include <stdio.h>

#define SUCCESS 0

void *potok(void *param) {

    for (int i = 0; i < 10; ++i) {
        printf("Thread %d\n", i);
    }
    pthread_exit((void *) SUCCESS);
}

int main() {

    pthread_t id;

    int status = pthread_create(&id, NULL, potok, NULL);

    if (status != SUCCESS) {
        pthread_exit((void *) status);
    }

    status = pthread_join(id, NULL);

    if (status != SUCCESS) {
        pthread_exit((void *) status);
    }

    for (int i = 0; i < 10; ++i) {
        printf("Main Thread %d\n", i);
    }

    pthread_exit((void *) SUCCESS);
}
