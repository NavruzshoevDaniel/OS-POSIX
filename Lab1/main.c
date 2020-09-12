#include <stdio.h>
#include <pthread.h>

#define SUCCESS 0

void *thread(void *param) {
    for (int i = 0; i < 10; ++i) {
        printf("Thread %d\n", i);
    }
    pthread_exit((void *) SUCCESS);
}

int main() {

    pthread_t id;

    int status = pthread_create(&id, NULL, thread, NULL);


    if (status != SUCCESS) {
        pthread_exit(status);
    }

    for (int i = 0; i < 10; ++i) {
        printf("Main Thread %d\n", i);
    }


    pthread_exit((void *) SUCCESS);;
}
