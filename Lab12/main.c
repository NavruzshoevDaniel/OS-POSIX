#include <stdio.h>
#include <pthread.h>
#include <zconf.h>

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t condition = PTHREAD_COND_INITIALIZER;

void *thread(void *param) {

    pthread_mutex_lock(&mutex);

    for (int i = 0; i < 10; ++i) {

        printf("Thread %d\n", i);
        pthread_cond_signal(&condition);
        printf("signal from child\n");
        pthread_cond_wait(&condition, &mutex);

    }
    printf("child");
    pthread_cond_signal(&condition);
    pthread_mutex_unlock(&mutex);
    pthread_exit((void *) 0);
}

int main() {
    pthread_t id;

    pthread_mutex_lock(&mutex);


    if (pthread_create(&id, NULL, thread, NULL)) {
        pthread_exit((void *) 0);
    }

    for (int i = 0; i < 10; ++i) {
        printf("Main Thread %d\n", i);
        pthread_cond_signal(&condition);
        printf("signal from parent\n");
        pthread_cond_wait(&condition, &mutex);
    }

    pthread_cond_signal(&condition);
    pthread_mutex_unlock(&mutex);

    pthread_exit((void *) 0);;
}
