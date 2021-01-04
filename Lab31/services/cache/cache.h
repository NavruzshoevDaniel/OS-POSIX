//
// Created by Daniel on 20.12.2020.
//

#ifndef LAB31_CACHE_H
#define LAB31_CACHE_H

#include "pthread.h"
#include "stdlib.h"
#include "stdio.h"
#include "string.h"
#include "../pthread/pthreadService.h"
#include "cacheList.h"

enum CacheStatus {
    DOWNLOADING,
    VALID,
    INVALID
} typedef CacheStatus;

struct CacheInfo {
    size_t allSize;
    size_t recvSize;
    pthread_mutex_t mutex;
    size_t readers;

    struct NodeCacheData *data;
    size_t numChunks;
    pthread_cond_t numChunksCondVar;
    pthread_mutex_t numChunksMutex;

    int writerId;
    char *url;
    enum CacheStatus status;
} typedef CacheEntry;

void setCacheStatus(CacheEntry *cacheInfo, CacheStatus status);

CacheStatus getCacheStatus(CacheEntry *cacheInfo);

int getCacheRecvSize(CacheEntry *cacheInfo);

int getCacheAllSize(CacheEntry *cacheInfo);

void setCacheAllSize(CacheEntry *cacheInfo, int allSize);

int putDataToCache(CacheEntry *cacheChunk, char *newData, int lengthNewData);

void destroyCache(CacheEntry *cache, int maxCacheSize);

int initCache(CacheEntry *cache, int maxCacheSize);

void makeCacheInvalid(CacheEntry *cache);

/**
 * If not using cache exits return index cache or else return -1
 * */
int searchNotUsingCacheAndSetDownloadingState(char *url, CacheEntry *cache, int cacheSize, int threadId);

/**
 * If free cache exits  return index cache or else return -1
 * */
int searchFreeCacheAndSetDownloadingState(char *url, CacheEntry *cache, int cacheSize, int threadId);

/**
 * If url exits return index cache or else return -1
 * */
int searchUrlInCache(char *url, CacheEntry *cache, int cacheSize);

int broadcastWaitingCacheClients(CacheEntry *cacheChunk);

//void getCopyData(CacheEntry *info,char **data,int *dataChunksSize, int index);


#endif //LAB31_CACHE_H
