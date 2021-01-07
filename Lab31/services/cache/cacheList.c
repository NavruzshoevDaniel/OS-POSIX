//
// Created by Daniel on 03.01.2021.
//

#include "cacheList.h"

void pushDataCacheBack(ListCacheData *list, char *data, int length) {
    NodeCacheData *tmp = (NodeCacheData *) malloc(sizeof(NodeCacheData));
    tmp->data = malloc(sizeof(char) * length);
    memcpy(tmp->data, data, sizeof(char) * length);
    tmp->lengthData = length;
    tmp->next = NULL;


    if (list->tail) {
        list->tail->next = tmp;
    }

    list->tail = tmp;

    if (list->head == NULL) {
        list->head = tmp;
    }
}

ListCacheData *initDataCacheList() {
    ListCacheData *tmp = (ListCacheData *) malloc(sizeof(ListCacheData));
    tmp->head = tmp->tail = NULL;
    return tmp;
}

NodeCacheData *getCacheNode(ListCacheData *list, int n) {
    int counter = 0;
    NodeCacheData *head = list->head;
    while (counter < n && head) {
        head = head->next;
        counter++;
    }
    return head;
}

void freeList(ListCacheData *list) {
    if (list == NULL || list->head==NULL) {
        return;
    }
    NodeCacheData *head = list->head;
    NodeCacheData *prev = NULL;
    while (head->next) {
        prev = head;
        head = head->next;
        free(prev->data);
        free(prev);
    }
    free(head);
}