#include <pthread.h>
#include <stdio.h>
#include <semaphore.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <alloca.h>
#include <unistd.h>

sem_t isEmpty;
sem_t isFull;
sem_t global;

struct Message {
    char message[81];
    struct Message *next;
    struct Message *prev;
};

struct Queue {
    struct Message *head;
    struct Message *tail;
    int isDroped;
};

void mymsginit(struct Queue *queue) {
    sem_init(&isEmpty, 0, 10);
    sem_init(&isFull, 0, 0);
    sem_init(&global, 0, 1);

    queue->head = NULL;
    queue->tail = NULL;
    queue->isDroped = 0;
}

void mymsqdrop(struct Queue *queue) {
    sem_wait(&global);
    queue->isDroped = 1;
    sem_post(&isEmpty);
    sem_post(&isFull);
    sem_post(&global);
}

void mymsgdestroy(struct Queue *queue) {
    sem_wait(&global);
    struct Message *tmp = queue->head;

    struct Message *del_tmp;
    while (tmp) {
        del_tmp = tmp;
        tmp = tmp->next;
        free(del_tmp);
    }

    sem_post(&global);

    sem_destroy(&global);
    sem_destroy(&isEmpty);
    sem_destroy(&isFull);
}

int mymsgget(struct Queue *queue, char *buf, size_t bufSize) {
    if (queue == NULL)
        return 0;

    sem_wait(&isFull);
    sem_wait(&global);

    if (queue->isDroped == 1) {
        sem_post(&global);
        sem_post(&isFull);
        return 0;
    }
    struct Message *res = queue->tail;
    if (queue->head == queue->tail) {
        queue->head = NULL;
        queue->tail = NULL;
    } else {
        queue->tail = queue->tail->prev;
        queue->tail->next = NULL;
    }
    strncpy(buf, res->message, bufSize);

    sem_post(&global);
    sem_post(&isEmpty);
    return bufSize;
}

int mymsgput(struct Queue *queue, char *msg) {
    if (queue == NULL)
        return 0;

    sem_wait(&isEmpty);
    sem_wait(&global);

    if (queue->isDroped == 1) {
        sem_post(&global);
        sem_post(&isEmpty);
        return 0;
    }

    struct Message *new_mes;
    new_mes = (struct Message *) malloc(sizeof(struct Message));
    new_mes->prev = NULL;
    new_mes->next = NULL;

    struct Message *tmp;
    sprintf(new_mes->message, "%s", "");
    strncat(new_mes->message, msg, 80);

    tmp = queue->head;
    queue->head = new_mes;
    if (tmp != NULL) {
        (queue->head)->next = tmp;
        tmp->prev = queue->head;
    }

    if (tmp == NULL) {
        queue->tail = new_mes;
    }

    sem_post(&global);
    sem_post(&isFull);
    return strlen(msg) > 80 ? 80 : strlen(msg);
}

//----------------------------------------------------------------------------------------------------------------------

void *producer(void *pq) {
    struct Queue *q = (struct Queue *) pq;
    int i;

    for (i = 0; i < 1000; i++) {
        char buf[40];
        sprintf(buf, "Message %d from thread %lu", i, pthread_self());
        if (mymsgput(q, buf) == 0)
            pthread_exit((void *) 0);
    }

    pthread_exit((void *) 0);
}

void *consumer(void *pq) {
    struct Queue *q = (struct Queue *) pq;
    int i;

    do {
        char buf[41];
        i = mymsgget(q, buf, sizeof(buf));
        if (i == 0) {
            break;
        } else {
            printf("Received by thread %lu: %s\n", pthread_self(), buf);
        }
    } while (1);

    pthread_exit((void *) 0);
}

int pleaseQuit = 0;

void sigintHandler(int sig) {
    pleaseQuit = 1;
    signal(sig, sigintHandler);
}

int main(int argc, char *argv[]) {
    struct Queue q;
    int nProducers, nConsumers;
    pthread_t *producers, *consumers;
    int i;

    if (argc < 2) {
        fprintf(stderr, "Usage: %s <nProducers> <nConsumers>\n", argv[0]);
        pthread_exit((void *) 0);
    }

    nProducers = atoi(argv[1]);
    nConsumers = atoi(argv[2]);

    if (nProducers == 0 || nConsumers == 0) {
        fprintf(stderr, "Usage: %s nProducers nConsumers\n", argv[0]);
        pthread_exit((void *) 0);
    }

    producers = (pthread_t *) malloc(sizeof(pthread_t) * nProducers);
    consumers = (pthread_t *) malloc(sizeof(pthread_t) * nConsumers);

    if (producers == NULL) {
        printf("Allocation failed! Producers\n");
        pthread_exit((void *) 0);
    }

    if (consumers == NULL) {
        printf("Allocation failed! Consumers\n");
        pthread_exit((void *) 0);
    }

    signal(SIGINT, sigintHandler);

    mymsginit(&q);
    for (i = 0; i < nProducers || i < nConsumers; i++) {
        if (i < nProducers) {
            if (pthread_create(&producers[i], NULL, producer, &q)) {
                printf("Error to create producer thread[%d]\n", i);
            }
        }
        if (i < nConsumers) {
            if (pthread_create(&consumers[i], NULL, consumer, &q)) {
                printf("Error to create consumer thread[%d]\n", i);
            }
        }
    }

    while (!pleaseQuit) pause();//pause() is THREAD-SAFE!!!

    mymsqdrop(&q);
    for (i = 0; i < nProducers || i < nConsumers; i++) {
        if (i < nProducers) {
            if (pthread_join(producers[i], NULL)) {
                printf("Error to join producer thread[%d]\n", i);
            }
        }
        if (i < nConsumers) {
            if (pthread_join(consumers[i], NULL)) {
                printf("Error to join consumer thread[%d]\n", i);
            }
        }
    }

    mymsgdestroy(&q);
    printf("All threads quit and queue destroyed\n");

    free(producers);
    free(consumers);

    pthread_exit((void *) 0);
}
