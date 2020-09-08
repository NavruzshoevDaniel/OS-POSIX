#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include "pthread.h"

#define SUCCESS 0

void *potok(void *param) {

    for (int i = 0; i < 10; ++i) {
        sleep(2);
        printf("Thread %d\n", i);
    }
    return SUCCESS;
}

int main() {

    pthread_t id;

    int status = pthread_create(&id, NULL, potok, NULL);


    if (status != SUCCESS) {
        exit(status);
    }

    status = pthread_join(id, NULL);

    if (status != SUCCESS) {
        exit(status);
    }


    for (int i = 0; i < 10; ++i) {
        sleep(1);
        printf("Main Thread %d\n",i);
    }


    return SUCCESS;
}
