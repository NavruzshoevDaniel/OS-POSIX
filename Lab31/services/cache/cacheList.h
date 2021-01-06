//
// Created by Daniel on 03.01.2021.
//

#ifndef LAB31_CACHELIST_H
#define LAB31_CACHELIST_H

#include "cache.h"

struct NodeCacheData {

    char *data;
    int lengthData;

    struct NodeCacheData *next;

} typedef NodeCacheData;

struct ListCacheData {
    NodeCacheData *head;
    NodeCacheData *tail;
} typedef ListCacheData;

ListCacheData *initDataCacheList();

void pushDataCacheBack(ListCacheData *head, char *data, int length);

NodeCacheData *getCacheNode(ListCacheData *list, int n);

void freeList(ListCacheData *list);

#endif //LAB31_CACHELIST_H
