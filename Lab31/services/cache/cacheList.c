//
// Created by Daniel on 03.01.2021.
//

#include "cacheList.h"

NodeCacheData *getLast(NodeCacheData *head) {
    if (head == NULL) {
        return NULL;
    }
    while (head->next) {
        head = head->next;
    }
    return head;
}


void pushDataCacheBack(NodeCacheData **head, char *data, int length) {
    while (*head) {
        head = &(*head)->next;
    }
    *head = malloc(sizeof(NodeCacheData));
    (*head)->data = malloc(sizeof(char) * length);
    memcpy((*head)->data, data, sizeof(char) * length);
    (*head)->lengthData = length;
    (*head)->next = NULL;
}

NodeCacheData *getCacheNode(NodeCacheData *head, int n) {
    int counter = 0;
    while (counter < n && head) {
        head = head->next;
        counter++;
    }
    return head;
}

void freeList(NodeCacheData **head) {
    NodeCacheData *prev = NULL;
    while ((*head)->next) {
        prev = (*head);
        (*head) = (*head)->next;
        free(prev->data);
        free(prev);
    }
    free(*head);
}