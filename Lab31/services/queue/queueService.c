//
// Created by Daniel on 19.12.2020.
//
#include "queueService.h"

Queue *createQueue() {
    Queue *queue = malloc(sizeof(Queue));

    queue->size = 0;
    queue->head = NULL;
    queue->tail = NULL;
    initMutex(&queue->queueMutex);
    initCondVariable(&queue->condVar);

    return queue;
}

void clearQueue(Queue *queue) {
    Entry *cur = queue->head;

    while (cur) {
        Entry *tmp = cur;
        cur = cur->next;
        free(tmp);
    }

    pthread_mutex_destroy(&queue->queueMutex);
    pthread_cond_destroy(&queue->condVar);
}

int getSocketFromQueue(Queue *queue) {

    if (queue->size == 0) {
        return -1;
    }

    Entry *temp = queue->head;

    if (NULL == queue->head->next) {
        queue->tail = NULL;
    }

    queue->head = queue->head->next;
    queue->size--;

    int result = temp->socket;
    free(temp);
    return result;
}

void putSocketInQueue(Queue *queue, int sock) {
    Entry *temp = malloc(sizeof(Entry));
    temp->socket = sock;
    temp->next = NULL;

    if (NULL == queue->tail) {
        queue->head = temp;
    } else {
        queue->tail->next = temp;
    }
    queue->tail = temp;
    queue->size++;

    pthread_cond_signal(&queue->condVar);
}

int isEmpty(const Queue *queue) {
    return queue->size == 0;
}



