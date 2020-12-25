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
#include "../connection/connection.h"

enum CacheStatus {
    DOWNLOADING,
    VALID,
    INVALID
};

struct CacheInfo {
    size_t allSize;
    size_t recvSize;
    pthread_mutex_t mutex;
    size_t readers;

    char **data;
    int *dataChunksSize;
    size_t numChunks;
    pthread_cond_t numChunksCondVar;
    pthread_mutex_t numChunksMutex;

    int writerId;
    char *url;
    enum CacheStatus status;
} typedef CacheInfo;

int putDataToCache(CacheInfo *cacheChunk, char *newData, int lengthNewData);

void destroyCache(CacheInfo *cache, int maxCacheSize);

int initCache(CacheInfo *cache, int maxCacheSize);

void makeCacheInvalid(CacheInfo *cache);

void freeDataChunks(char **data, size_t numChunks);

/**
 * If not using cache exits return index cache or else return -1
 * */
int searchNotUsingCacheAndSetDownloadingState(char *url, CacheInfo *cache, int cacheSize, int threadId);

/**
 * If free cache exits  return index cache or else return -1
 * */
int searchFreeCacheAndSetDownloadingState(char *url, CacheInfo *cache, int cacheSize, int threadId);

/**
 * If url exits return index cache or else return -1
 * */
int searchUrlInCache(char *url, CacheInfo *cache, int cacheSize);

int broadcastWaitingCacheClients(CacheInfo *cacheChunk);


#endif //LAB31_CACHE_H
