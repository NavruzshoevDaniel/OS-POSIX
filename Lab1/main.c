#include <stdio.h>
#include <pthread.h>

void *thread(void *param) {
    for (int i = 0; i < 10; ++i) {
        printf("Thread %d\n", i);
    }
    pthread_exit((void *)0);
}

int main() {
    pthread_t id;
    if (pthread_create(&id, NULL, thread, NULL)) {
        return 1;
    }
    for (int i = 0; i < 10; ++i) {
        printf("Main Thread %d\n", i);
    }
    pthread_exit((void *)0);;
}
