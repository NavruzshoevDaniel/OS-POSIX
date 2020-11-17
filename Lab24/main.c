#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <alloca.h>
#include <unistd.h>
#include <time.h>

pthread_mutex_t mutex;
pthread_mutex_t doubleMutex;
pthread_cond_t cond;

void logger(const char* tag, const char* message) {
    time_t now;
    time(&now);
    if(strcmp(tag,"mymsgput")==0){
        printf("\033[0;32m %s [%s] [thread:%d]: %s\n\033[0m", ctime(&now), tag, pthread_self(), message);
    }
    else{
        printf("\033[0;34m %s [%s] [thread:%d]: %s\n\033[0m", ctime(&now), tag, pthread_self(), message);
    }

}
struct Message {
    char message[81];
    struct Message* next;
    struct Message* prev;
};

struct Queue {
    struct Message* head;
    struct Message* tail;
    int isDroped;
    int mesCount;
};

void mymsginit(struct Queue* queue) {
    pthread_mutex_init(&mutex, NULL);
    pthread_mutex_init(&doubleMutex, NULL);
    pthread_cond_init(&cond, NULL);
    queue->head = NULL;
    queue->tail = NULL;
    queue->isDroped = 0;
    queue->mesCount = 0;
}

void mymsqdrop(struct Queue* queue) {
    pthread_mutex_lock(&mutex);
    queue->isDroped = 1;
    pthread_cond_broadcast(&cond);
    pthread_mutex_unlock(&mutex);
}

void mymsgdestroy(struct Queue* queue) {
    struct Message* tmp = queue->head;
    pthread_mutex_lock(&mutex);
    struct Message* del_tmp;
    while(tmp) {
        del_tmp  = tmp;
        tmp = tmp->next;
        free(del_tmp);
    }
    pthread_mutex_unlock(&mutex);
    pthread_cond_destroy(&cond);
    pthread_mutex_destroy(&mutex);
    pthread_mutex_destroy(&doubleMutex);
}

int mymsgget(struct Queue* queue, char* buf, size_t bufSize) {
    if(queue == NULL){
        return 0;
    }

    pthread_mutex_lock(&mutex);
    logger("mymsget", "block mutex");
    while(queue->mesCount == 0 && queue->isDroped != 1){
        pthread_mutex_lock(&doubleMutex);
        logger("mymsget", "wait and unlock mutex");
        pthread_mutex_unlock(&doubleMutex);
        pthread_cond_wait(&cond, &mutex);
        logger("mymsget", "lock mutex after wait");

    }


    if(queue->isDroped == 1) {
        logger("mymsget", "unlock mutex");
        pthread_mutex_unlock(&mutex);
        return 0;
    }
    struct Message* res = queue->tail;
    if(queue->head == queue->tail) {
        queue->head = NULL;
        queue->tail = NULL;
    }
    else {
        queue->tail = queue->tail->prev;
        queue->tail->next = NULL;
    }
    strncpy(buf, res->message, bufSize);
    printf("curMesCount=%d\n",queue->mesCount);
    if(queue->mesCount == 10){
        pthread_cond_broadcast(&cond);
        logger("mymsget", "broadcast signal that mesCount<10");
    }

    (queue->mesCount)--;
    pthread_mutex_unlock(&mutex);
    return 1;
}

int mymsgput(struct Queue* queue, char* msg) {
    if(queue == NULL){
        return 0;
    }

    pthread_mutex_lock(&mutex);
    logger("mymsgput", "block mutex");

    while(queue->mesCount >= 10 && queue->isDroped != 1){
        logger("mymsgput", "wait and unlock mutex");
        pthread_cond_wait(&cond, &mutex);
        logger("mymsgput", "lock mutex after wait");
    }


    if(queue->isDroped == 1) {
        logger("mymsgput", "its dropped and unlock mutex");
        pthread_mutex_unlock(&mutex);
        return 0;
    }

    struct Message* new_mes;
    new_mes = (struct Message*)malloc(sizeof(struct Message));
    new_mes->prev = NULL;
    new_mes->next = NULL;
    printf("curMesCount=%d\n",queue->mesCount);
    struct Message* tmp;

    sprintf(new_mes->message, "%s", "");
    strncat(new_mes->message, msg, 80);

    tmp = queue->head;
    queue->head = new_mes;
    if(tmp != NULL) {
        (queue->head)->next = tmp;
        tmp->prev = queue->head;
    }

    if(tmp == NULL) {
        queue->tail = new_mes;
    }

    if(queue->mesCount == 0){
        logger("mymsgput", "broadcast signal that mesCount>0");
        pthread_cond_broadcast(&cond);
    }

    (queue->mesCount)++;
    pthread_mutex_unlock(&mutex);
    return 1;
}

//----------------------------------------------------------------------------------------------------------------------

void *producer(void *pq) {
    struct Queue *q=(struct Queue*)pq;
    int i;

    for(i=0; i<1000; i++) {
        char buf[40];
        sprintf(buf, "Message %d from thread %lu", i, pthread_self());
        if (mymsgput(q, buf) == 0) {
            pthread_exit((void*)0);
        }
    }

    pthread_exit((void*)0);
}

void *consumer(void *pq) {
    struct Queue *q=( struct Queue*)pq;
    int i;

    do {
        char buf[41];
        i=mymsgget(q, buf, sizeof(buf));
        if (i==0) {
            break;
        }
        else {
            printf("Received by thread %lu: %s\n", pthread_self(), buf);
        }
    } while(1);
    pthread_exit((void*)0);
}

int pleaseQuit = 0;

void sigintHandler(int sig) {
    pleaseQuit = 1;
    signal(sig, sigintHandler);
}


int main(int argc, char* argv[]) {
    struct Queue q;
    int nProducers, nConsumers;
    pthread_t *producers, *consumers;
    int i;

    if (argc < 2) {
        fprintf(stderr, "Usage: %s nProducers nConsumers\n", argv[0]);
        pthread_exit((void *)0);
    }
    nProducers = atoi(argv[1]);
    nConsumers = atoi(argv[2]);
    if (nProducers == 0 || nConsumers == 0) {
        fprintf(stderr, "Usage: %s nProducers nConsumers\n", argv[0]);
        pthread_exit((void *)0);
    }

    producers = (pthread_t*)malloc(sizeof(pthread_t) * nProducers);
    consumers = (pthread_t*)malloc(sizeof(pthread_t) * nConsumers);

    if(producers == NULL){
        printf("Allocation failed! Producers\n");
        pthread_exit((void *)0);
    }

    if(consumers == NULL){
        printf("Allocation failed! Consumers\n");
        pthread_exit((void *)0);
    }

    signal(SIGINT, sigintHandler);
    mymsginit(&q);

    for(i=0; i < nProducers || i < nConsumers; i++) {
        if (i < nProducers) {
            if(pthread_create(&producers[i], NULL, producer, &q)){
                printf("Error to create producer thread[%d]\n", i);
            }
        }
        if (i < nConsumers) {
            if(pthread_create(&consumers[i], NULL, consumer, &q)){
                printf("Error to create consumer thread[%d]\n", i);
            }
        }
    }

    while (!pleaseQuit) pause();

    mymsqdrop(&q);
    for(i=0; i < nProducers || i < nConsumers; i++) {
        if(i < nProducers) {
            pthread_join(producers[i], NULL);
        }
        if (i < nConsumers) {
            pthread_join(consumers[i], NULL);
        }
    }
    mymsgdestroy(&q);
    printf("All threads quit and queue destroyed\n");

    free(producers);
    free(consumers);

    pthread_exit((void *)0);
}
