//
// Created by Daniel on 03.01.2021.
//

#include "cacheList.h"

void pushDataCacheBack(NodeCacheData **head, char *data, int length) {
    while (*head) {
        head = &(*head)->next;
    }
    NodeCacheData *new = malloc(sizeof(NodeCacheData));
    new->data = malloc(sizeof(char) * length);
    memcpy(new->data, data, sizeof(char) * length);
    new->lengthData = length;
    new->next = NULL;
    (*head)=new;
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