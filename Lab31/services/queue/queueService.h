//
// Created by Daniel on 19.12.2020.
//

#ifndef LAB31_QUEUESERVICE_H
#define LAB31_QUEUESERVICE_H

#include <pthread.h>
#include "../pthread/pthreadService.h"

struct Entry {
    int socket;
    struct Entry *next;
} typedef Entry;

struct Queue {
    int size;

    Entry *head;
    Entry *tail;

    pthread_mutex_t queueMutex;
    pthread_cond_t condVar;

} typedef Queue;

Queue *createQueue();

void clearQueue(Queue *queue);

int getSocketFromQueue(Queue *queue);

void putSocketInQueue(Queue *queue, int sock);

int isEmpty(const Queue *queue);

#endif //LAB31_QUEUESERVICE_H
