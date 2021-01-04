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

void pushDataCacheBack(NodeCacheData **head, char *data, int length);

NodeCacheData *getCacheNode(NodeCacheData *head, int n);

void freeList(NodeCacheData **head);

#endif //LAB31_CACHELIST_H
