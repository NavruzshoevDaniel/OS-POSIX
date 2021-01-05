//
// Created by Daniel on 20.12.2020.
//

#include "cache.h"
#include "../logger/logging.h"

/**
 * if url exits set READ_FROM_CACHE_WRITE_CLIENT state connection and return index cache
 * or else return -1
 * */
int searchUrlInCache(char *url, CacheEntry *cache, int cacheSize) {
    for (int j = 0; j < cacheSize; j++) {
        infoPrintf("searchUrlInCache lock...");
        pthread_mutex_lock(&cache[j].mutex);

        if (cache[j].url != NULL && strcmp(cache[j].url, url) == 0) {
            if (cache[j].status == VALID || cache[j].status == DOWNLOADING) {
                cache[j].readers++;
                pthread_mutex_unlock(&cache[j].mutex);
                infoPrintf("searchUrlInCacheEND");
                return j;
            }

            pthread_mutex_unlock(&cache[j].mutex);
            // infoPrintf("searchUrlInCacheEND");
        } else {
            pthread_mutex_unlock(&cache[j].mutex);
            // infoPrintf("searchUrlInCacheEND ");
        }
    }
    return -1;
}

/**
 * if free cache exits set WRITE_TO_SERVER state connection and return index cache
 * or else return -1
 * */
int searchFreeCacheAndSetDownloadingState(char *url,
                                          CacheEntry *cache,
                                          int cacheSize,
                                          int threadId) {
    for (int j = 0; j < cacheSize; j++) {

        infoPrintf("searchFreeCacheAndSetDownloadingState lock...");
        pthread_mutex_lock(&cache[j].mutex);
        if (cache[j].url == NULL) {
            infoPrintf("(%d)SEARCH_CACHE: found free cache id=%d", threadId, j);
            cache[j].readers = 1;
            cache[j].status = DOWNLOADING;
            cache[j].writerId = threadId;
            cache[j].data = NULL;
            cache[j].numChunks = 0;
            cache[j].allSize = 0;
            cache[j].recvSize = 0;
            cache[j].url = (char *) malloc(sizeof(char) * strlen(url) + 1);
            memcpy(cache[j].url, url, sizeof(char) * strlen(url) + 1);

            pthread_mutex_unlock(&cache[j].mutex);
            infoPrintf("searchFreeCacheAndSetDownloadingStateEND");
            return j;
        } else {
            pthread_mutex_unlock(&cache[j].mutex);
            //infoPrintf("searchFreeCacheAndSetDownloadingStateEND");
        }
    }
    return -1;
}

/**
 * if not using cache exits set WRITE_TO_SERVER state connection and return index cache
 * or else return -1
 * */
int searchNotUsingCacheAndSetDownloadingState(char *url,
                                              CacheEntry *cache,
                                              int cacheSize,
                                              int threadId) {
    for (int j = 0; j < cacheSize; j++) {
        infoPrintf("searchNotUsingCacheAndSetDownloadingState...");
        pthread_mutex_lock(&cache[j].mutex);

        if (cache[j].readers == 0 || cache[j].status == INVALID) {
            printf("(%d)SEARCH_CACHE: found not using cache id=%d", threadId, j);
            cache[j].readers = 1;
            cache[j].status = DOWNLOADING;
            cache[j].writerId = threadId;
            cache[j].data = NULL;
            cache[j].numChunks = 0;
            cache[j].allSize = 0;
            cache[j].recvSize = 0;

            freeList(&cache[j].data);
            free(cache[j].url);
            cache[j].url = (char *) malloc(sizeof(char) * sizeof(url));
            memcpy(cache[j].url, url, sizeof(char) * sizeof(url));

            pthread_mutex_unlock(&cache[j].mutex);
            //infoPrintf("searchNotUsingCacheAndSetDownloadingState ");
            return j;
        } else {
            pthread_mutex_unlock(&cache[j].mutex);
            //infoPrintf("searchNotUsingCacheAndSetDownloadingStateEND");
        }
    }
    return -1;
}

void makeCacheInvalid(CacheEntry *cache) {
    setCacheStatus(cache, INVALID);
    cache->writerId = -1;
    pthread_cond_broadcast(&cache->numChunksCondVar);
}

int initCache(CacheEntry *cache, const int maxCacheSize) {

    bool erMS, erCVC, erMC;
    for (int i = 0; i < maxCacheSize; i++) {

        cache[i].allSize = 0;
        cache[i].recvSize = 0;
        erMS = initMutex(&cache[i].mutex);

        cache[i].readers = 0;
        cache[i].data = NULL;
        cache[i].numChunks = 0;
        erCVC = initCondVariable(&cache[i].numChunksCondVar);
        erMC = initMutex(&cache[i].numChunksMutex);

        cache[i].writerId = -1;
        cache[i].url = NULL;
    }
    return erMS && erCVC && erMC;
}

void destroyCache(CacheEntry *cache, const int maxCacheSize) {

    for (int i = 0; i < maxCacheSize; i++) {

        pthread_mutex_destroy(&cache[i].mutex);
        freeList(&cache[i].data);
        pthread_cond_destroy(&cache[i].numChunksCondVar);
        pthread_mutex_destroy(&cache[i].numChunksMutex);

        free(cache[i].url);
    }
    printf("destroy cache");
}

int putDataToCache(CacheEntry *cacheChunk, char *newData, int lengthNewData) {
    pushDataCacheBack(&cacheChunk->data, newData, lengthNewData);
    cacheChunk->recvSize += lengthNewData;
    pthread_mutex_lock(&cacheChunk->numChunksMutex);
    cacheChunk->numChunks++;
    pthread_mutex_unlock(&cacheChunk->numChunksMutex);
    return 0;
}

void setCacheStatus(CacheEntry *cacheInfo, CacheStatus status) {
    infoPrintf("setCacheStatus...");
    pthread_mutex_lock(&cacheInfo->mutex);
    cacheInfo->status = status;
    pthread_mutex_unlock(&cacheInfo->mutex);
    infoPrintf("setCacheStatusEND");
}

CacheStatus getCacheStatus(CacheEntry *cacheInfo) {
    infoPrintf("getCacheStatus");
    CacheStatus returnStatus;
    pthread_mutex_lock(&cacheInfo->mutex);
    returnStatus = cacheInfo->status;
    pthread_mutex_unlock(&cacheInfo->mutex);
    infoPrintf("getCacheStatusEND");
    return returnStatus;
}

int getCacheRecvSize(CacheEntry *cacheInfo) {
    return cacheInfo->recvSize;
}


int getCacheAllSize(CacheEntry *cacheInfo) {
    CacheStatus returnAllSize;
    returnAllSize = cacheInfo->allSize;
    return returnAllSize;
}

void setCacheAllSize(CacheEntry *cacheInfo, int allSize) {
    cacheInfo->allSize = allSize;
}

int broadcastWaitingCacheClients(CacheEntry *cacheChunk) {
    pthread_cond_broadcast(&cacheChunk->numChunksCondVar);
}
